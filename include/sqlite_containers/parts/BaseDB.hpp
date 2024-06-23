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

	/// \brief Base class for SQLite database management.
	class BaseDB {
	public:

		/// \brief Default constructor.
		BaseDB() = default;

		/// \brief Destructor.
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
		/// \return Current configuration settings.
		Config get_config() {
			std::lock_guard<std::mutex> locker(m_config_mutex);
			return m_config;
		}

		/// \brief Connects to the database.
		/// Initializes a connection to the database by creating necessary directories, opening the database, creating tables, and setting up database parameters.
		void connect() {
			std::lock_guard<std::mutex> locker(m_sqlite_mutex);
			if (!m_sqlite_db && !m_config_update) {
				throw sqlite_exception("Database connection already exists and no configuration update required.");
			}
			if (m_sqlite_db) {
				if (!m_config_update) return;
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
		void connect(const Config& config) {
			set_config(config);
			connect();
		}

		/// \brief Disconnects from the database.
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
				throw sqlite_exception("An unspecified error occurred while waiting for async operations.");
			}
		}

		/// \brief Begins a database transaction.
		/// \param mode Transaction mode (defaults to DEFERRED).
		/// \throws sqlite_exception if the execution fails.
		void begin(const TransactionMode &mode = TransactionMode::DEFERRED) {
			std::lock_guard<std::mutex> locker(m_sqlite_mutex);
			db_begin(mode);
		}

		/// \brief Commits the current transaction.
		/// \throws sqlite_exception if the execution fails.
		void commit() {
			std::lock_guard<std::mutex> locker(m_sqlite_mutex);
			db_commit();
		}

		/// \brief Rolls back the current transaction.
		/// \throws sqlite_exception if the execution fails.
		void rollback() {
			std::lock_guard<std::mutex> locker(m_sqlite_mutex);
			db_rollback();
		}

		/// \brief Processes asynchronous database requests.
		virtual void process() {};

	protected:
		sqlite3*	m_sqlite_db = nullptr;
		std::mutex	m_sqlite_mutex;

		/// \brief Begins a database transaction.
		/// \param mode Transaction mode (defaults to DEFERRED).
		/// \throws sqlite_exception if the execution fails.
		void db_begin(const TransactionMode &mode = TransactionMode::DEFERRED) {
			m_stmt_begin[static_cast<size_t>(mode)].execute(m_sqlite_db);
		}

		/// \brief Commits the current transaction.
		/// \throws sqlite_exception if the execution fails.
		void db_commit() {
			m_stmt_commit.execute(m_sqlite_db);
		}

		/// \brief Rolls back the current transaction.
		/// \throws sqlite_exception if the execution fails.
		void db_rollback() {
			m_stmt_rollback.execute(m_sqlite_db);
		}

	private:

		std::array<SqliteStmt, 3> m_stmt_begin;
		SqliteStmt	m_stmt_commit;
		SqliteStmt	m_stmt_rollback;

		Config		m_config_new;
		Config		m_config;
		std::mutex	m_config_mutex;
		std::atomic<bool> m_config_update = ATOMIC_VAR_INIT(false);

		std::shared_future<void> m_future;

		/// \brief Create directories for the database.
		/// \param config Configuration settings.
		/// Creates necessary directories for the database file if they do not exist.
		void db_create_directories(const Config &config) {
			// Create all necessary subdirectories based on the file path.
			fs::path file_path(config.db_path);
			fs::path parent_dir = file_path.parent_path();
			if (parent_dir.empty()) return;
			if (!fs::exists(parent_dir)) {
				if (!fs::create_directories(parent_dir)) {
					throw sqlite_exception("Failed to create directories for path: " + parent_dir.string());
				}
			}
		}

		/// \brief Open the database.
		/// \param config Configuration settings.
		/// Opens the database with the flags specified in the configuration.
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

		/// \brief Initialize the database.
		/// \param config Configuration settings.
		/// Sets database parameters such as busy timeout, page size, cache size, journal mode, and other settings.
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

		/// \brief Create a table in the database.
		/// \param config Configuration settings.
		/// A virtual method that must be implemented in derived classes to create tables in the database.
		virtual void db_create_table(const Config &config) = 0;

		/// \brief Called after the database is opened.
		/// A virtual method that can be overridden in derived classes to perform actions after the database is opened.
		virtual void on_db_open() {}

		/// \brief Called before the database is closed.
		/// A virtual method that can be overridden in derived classes to perform actions before the database is closed.
		virtual void on_db_close() {}

	}; // BaseDB

}; // namespace sqlite_containers
