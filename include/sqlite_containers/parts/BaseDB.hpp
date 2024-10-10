#pragma once

/// \file BaseDB.hpp
/// \brief Base class for SQLite database management in sqlite_containers.

#include "Config.hpp"
#include "Utils.hpp"
#include "SqliteStmt.hpp"
#include <filesystem>
#include <future>
#include <mutex>
#include <atomic>

#if SQLITE_THREADSAFE != 1
#error "The project must be built for sqlite multithreading! Set the SQLITE_THREADSAFE=1"
#endif

namespace sqlite_containers {
    namespace fs = std::filesystem;

    /// \class BaseDB
    /// \brief Base class for SQLite database management.
    class BaseDB {
    public:

        /// \brief Default constructor.
        BaseDB() = default;

        /// \brief Destructor.
        /// Disconnects from the database if connected.
        virtual ~BaseDB() {
            disconnect();
        }

        /// \brief Sets the configuration for the database.
        /// \param config Configuration settings for the database.
        void set_config(const Config& config) {
            std::lock_guard<std::mutex> locker(m_config_mutex);
            m_config_new = config;
            m_config_update = true;
        }

        /// \brief Gets the current configuration of the database.
        /// \return The current configuration settings.
        Config get_config() const {
            std::lock_guard<std::mutex> locker(m_config_mutex);
            return m_config;
        }

        /// \brief Connects to the database using the current configuration.
        /// Initializes a connection to the database by creating necessary directories, opening the database, creating tables, and setting up database parameters.
        /// \throws sqlite_exception if connection fails.
        void connect() {
            std::lock_guard<std::mutex> locker(m_sqlite_mutex);
            if (!m_sqlite_db && !m_config_update) {
                throw sqlite_exception("Database connection already exists and no configuration update required.");
            }
            if (m_sqlite_db) {
                if (!m_config_update) return;
                on_db_close();
                sqlite3_close_v2(m_sqlite_db);
                m_sqlite_db = nullptr;
            }

            std::unique_lock<std::mutex> config_locker(m_config_mutex);
            m_config = m_config_new;
            m_config_update = false;
            config_locker.unlock();

            try {
                db_create_directories(m_config);
                db_open(m_config);
                on_db_open();
                db_create_table(m_config);
                db_init(m_config);
            } catch(const sqlite_exception &e) {
                sqlite3_close_v2(m_sqlite_db);
                m_sqlite_db = nullptr;
                throw e;
            } catch(...) {
                sqlite3_close_v2(m_sqlite_db);
                m_sqlite_db = nullptr;
                throw sqlite_exception("An unspecified error occurred in the database operation.");
            }
        }

        /// \brief Connects to the database with the given configuration.
        /// \param config Configuration settings for the database.
        /// \throws sqlite_exception if connection fails.
        void connect(const Config& config) {
            set_config(config);
            connect();
        }

        /// \brief Disconnects from the database.
        /// \throws sqlite_exception if disconnect fails.
        void disconnect() {
            std::lock_guard<std::mutex> locker(m_sqlite_mutex);
            if (!m_sqlite_db) return;

            on_db_close();
            sqlite3_close_v2(m_sqlite_db);
            m_sqlite_db = nullptr;

            try {
                if (m_future.valid()) {
                    while (m_future.wait_for(std::chrono::milliseconds(1)) == std::future_status::timeout) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    }
                    m_future.get();
                }
            } catch(const sqlite_exception &e) {
                throw e;
            } catch(const std::exception &e) {
                throw sqlite_exception(e.what());
            } catch(...) {
                throw sqlite_exception("Error occurred during async operation.");
            }
        }

        /// \brief Begins a database transaction.
        /// \param mode Transaction mode (default: DEFERRED).
        /// \throws sqlite_exception if the transaction fails.
        void begin(const TransactionMode &mode = TransactionMode::DEFERRED) {
            std::lock_guard<std::mutex> locker(m_sqlite_mutex);
            db_begin(mode);
        }

        /// \brief Commits the current transaction.
        /// \throws sqlite_exception if the commit fails.
        void commit() {
            std::lock_guard<std::mutex> locker(m_sqlite_mutex);
            db_commit();
        }

        /// \brief Rolls back the current transaction.
        /// \throws sqlite_exception if the rollback fails.
        void rollback() {
            std::lock_guard<std::mutex> locker(m_sqlite_mutex);
            db_rollback();
        }

        /// \brief Executes an operation inside a transaction.
        /// \param operation The operation to execute.
        /// \param mode The transaction mode.
        /// \throws sqlite_exception if an error occurs during execution.
        template<typename Func>
        void execute_in_transaction(Func operation, const TransactionMode& mode) {
            std::lock_guard<std::mutex> locker(m_sqlite_mutex);
            try {
                db_begin(mode);
                operation();
                db_commit();
            } catch(const sqlite_exception &e) {
                db_rollback();
                throw e;
            } catch(const std::exception &e) {
                db_rollback();
                throw sqlite_exception(e.what());
            } catch(...) {
                db_rollback();
                throw sqlite_exception("Unknown error occurred.");
            }
        }

        /// \brief Processes asynchronous database requests (can be overridden).
        virtual void process() {};

    protected:
        sqlite3*            m_sqlite_db = nullptr;
        mutable std::mutex  m_sqlite_mutex;

        /// \brief Begins a transaction with the given mode.
        /// \param mode Transaction mode (defaults to DEFERRED).
        /// \throws sqlite_exception if the transaction fails.
        void db_begin(const TransactionMode &mode = TransactionMode::DEFERRED) {
            m_stmt_begin[static_cast<size_t>(mode)].execute(m_sqlite_db);
        }

        /// \brief Commits the current transaction.
        /// \throws sqlite_exception if the commit fails.
        void db_commit() {
            m_stmt_commit.execute(m_sqlite_db);
        }

        /// \brief Rolls back the current transaction.
        /// \throws sqlite_exception if the rollback fails.
        void db_rollback() {
            m_stmt_rollback.execute(m_sqlite_db);
        }

        /// \brief Handles an exception by resetting and clearing bindings of prepared SQL statements.
        /// \param ex A pointer to the current exception (`std::exception_ptr`) that needs to be handled.
        /// \param stmts A vector of pointers to `SqliteStmt` objects, which will be reset and cleared.
        /// \param message A custom error message used when an unknown exception is encountered. Defaults to "Unknown error occurred."
        /// \throws sqlite_exception If an unknown exception or `std::exception` is encountered, it will be rethrown as `sqlite_exception`. If the caught exception is already `sqlite_exception`, it will be rethrown as is.
        void db_handle_exception(
                std::exception_ptr ex,
                const std::vector<SqliteStmt*>& stmts,
                const std::string& message = "Unknown error occurred.") const {
            try {
                for (auto* stmt : stmts) {
                    stmt->reset();
                    stmt->clear_bindings();
                }
                if (ex) {
                    std::rethrow_exception(ex);
                }
            } catch (const sqlite_exception&) {
                throw;
            } catch (const std::exception& e) {
                throw sqlite_exception(e.what());
            } catch (...) {
                throw sqlite_exception(message);
            }
        }

    private:

        std::array<SqliteStmt, 3> m_stmt_begin;
        SqliteStmt          m_stmt_commit;
        SqliteStmt          m_stmt_rollback;

        Config              m_config_new;
        Config              m_config;
        mutable std::mutex  m_config_mutex;
        std::atomic<bool>   m_config_update = ATOMIC_VAR_INIT(false);

        std::shared_future<void> m_future;

        /// \brief Creates necessary directories for the database.
        /// This method checks if the parent directory of the database file exists, and if not, attempts to create it.
        /// \param config Configuration settings, including the path to the database file.
        /// \throws sqlite_exception If the directories cannot be created.
        void db_create_directories(const Config &config) {
            fs::path file_path(config.db_path);
            fs::path parent_dir = file_path.parent_path();
            if (parent_dir.empty()) return;
            if (!fs::exists(parent_dir)) {
                if (!fs::create_directories(parent_dir)) {
                    throw sqlite_exception("Failed to create directories for path: " + parent_dir.string());
                }
            }
        }

        /// \brief Opens the database with the specified configuration.
        /// \param config Configuration settings.
        /// \throws sqlite_exception if the database fails to open.
        void db_open(const Config &config) {
            int flags = 0;
            flags |= config.read_only ? SQLITE_OPEN_READONLY : (SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
            flags |= config.use_uri ? SQLITE_OPEN_URI : 0;
            flags |= config.in_memory ? SQLITE_OPEN_MEMORY : 0;
            flags |= SQLITE_OPEN_FULLMUTEX;
            int err = 0;
            const char* db_name = config.in_memory ? ":memory:" : config.db_path.c_str();
            if ((err = sqlite3_open_v2(db_name, &m_sqlite_db, flags, nullptr)) != SQLITE_OK) {
                std::string error_message = "Cannot open database: ";
                error_message += sqlite3_errmsg(m_sqlite_db);
                error_message += " (Error code: ";
                error_message += std::to_string(err);
                error_message += ")";
                throw sqlite_exception(error_message);
            }
        }

        /// \brief Initializes the database with the given configuration.
        /// Sets database parameters such as busy timeout, page size, cache size, journal mode, and other settings.
        /// \param config Configuration settings.
        void db_init(const Config &config) {
            execute(m_sqlite_db, "PRAGMA busy_timeout = " + std::to_string(config.busy_timeout) + ";");
            execute(m_sqlite_db, "PRAGMA page_size = " + std::to_string(config.page_size) + ";");
            execute(m_sqlite_db, "PRAGMA cache_size = " + std::to_string(config.cache_size) + ";");
            execute(m_sqlite_db, "PRAGMA analysis_limit = " + std::to_string(config.analysis_limit) + ";");
            execute(m_sqlite_db, "PRAGMA wal_autocheckpoint = " + std::to_string(config.wal_autocheckpoint) + ";");
            execute(m_sqlite_db, "PRAGMA journal_mode = " + to_string(config.journal_mode) + ";");
            execute(m_sqlite_db, "PRAGMA synchronous = " + to_string(config.synchronous) + ";");
            execute(m_sqlite_db, "PRAGMA locking_mode = " + to_string(config.locking_mode) + ";");
            execute(m_sqlite_db, "PRAGMA auto_vacuum = " + to_string(config.auto_vacuum_mode) + ";");

            for (size_t i = 0; i < m_stmt_begin.size(); ++i) {
                m_stmt_begin[i].init(m_sqlite_db, "BEGIN " + to_string(static_cast<TransactionMode>(i)) + " TRANSACTION");
            }
            m_stmt_commit.init(m_sqlite_db, "COMMIT");
            m_stmt_rollback.init(m_sqlite_db, "ROLLBACK");

            if (config.user_version > 0) {
                execute(m_sqlite_db, "PRAGMA user_version = " + std::to_string(config.user_version) + ";");
            }
            if (config.use_async) {
                m_future = std::async(std::launch::async,
                        [this] {
                    process();
                }).share();
            }
        }

    protected:

        /// \brief Creates tables in the database.
        /// Must be implemented in derived classes.
        /// \param config Configuration settings.
        virtual void db_create_table(const Config &config) = 0;

        /// \brief Called after the database is opened.
        /// Can be overridden in derived classes.
        virtual void on_db_open() {}

        /// \brief Called before the database is closed.
        /// Can be overridden in derived classes.
        virtual void on_db_close() {}

    }; // BaseDB

}; // namespace sqlite_containers
