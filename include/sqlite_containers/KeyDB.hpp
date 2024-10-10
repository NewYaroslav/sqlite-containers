#pragma once

/// \file KeyDB.hpp
/// \brief Declaration of the KeyDB class for managing keys in a SQLite database.

#include "parts/BaseDB.hpp"

namespace sqlite_containers {

    /// \class KeyDB
    /// \brief Template class for managing keys in a SQLite database.
    /// \tparam KeyT Type of the keys.
    /// \details This class supports various container types, including `std::set`,
    /// `std::unordered_set`, `std::vector`, and `std::list`. The class allows insertion,
    /// retrieval, and removal of keys from a SQLite database while maintaining transactional integrity.
    template<class KeyT>
    class KeyDB final : public BaseDB {
    public:

        /// \brief Default constructor.
        KeyDB() : BaseDB() {}

        /// \brief Constructor with configuration.
        /// \param config Configuration settings for the database.
        KeyDB(const Config& config) : BaseDB() {
            set_config(config);
        }

        /// \brief Destructor.
        ~KeyDB() override final = default;

        // --- Operators ---

        /// \brief Assigns a container (e.g., std::set, std::unordered_set, std::vector, or std::list) to the database.
        /// \tparam ContainerT The type of the container (e.g., std::set, std::unordered_set).
        /// \param container The container with keys to be inserted into the database.
        /// \return Reference to this KeyDB.
        /// \throws sqlite_exception if an SQLite error occurs.
        /// \note The transaction mode is taken from the database configuration.
        template<template <class...> class ContainerT>
        KeyDB& operator=(const ContainerT<KeyT>& container) {
            // Get the default transaction mode from the configuration
            auto txn_mode = get_config().default_txn_mode;

            execute_in_transaction([this, &container]() {
                db_reconcile(container);
            }, txn_mode);  // Use transaction mode from the configuration
            return *this;
        }

        /// \brief Loads all keys from the database into a container (e.g., std::set, std::unordered_set, std::vector, or std::list).
        /// \tparam ContainerT The type of the container (e.g., std::set, std::unordered_set, std::vector, or std::list).
        /// \return A container populated with all keys from the database.
        /// \throws sqlite_exception if an SQLite error occurs.
        /// \note The transaction mode is taken from the database configuration.
        template<template <class...> class ContainerT = std::set>
        ContainerT<KeyT> operator()() {
            ContainerT<KeyT> container;
            // Get the default transaction mode from the configuration
            auto txn_mode = get_config().default_txn_mode;

            execute_in_transaction([this, &container]() {
                db_load(container);
            }, txn_mode);  // Use transaction mode from the configuration
            return container;
        }

        // --- Existing methods ---

        /// \brief Loads data from the database into the container.
        /// \tparam ContainerT Template for the container type (vector, deque, list, set or unordered_set).
        /// \param container Container to be synchronized with database content.
        /// \throws sqlite_exception if an SQLite error occurs.
        template<template <class...> class ContainerT>
        void load(ContainerT<KeyT>& container) {
            std::lock_guard<std::mutex> locker(m_sqlite_mutex);
            db_load(container);
        }

        /// \brief Loads data from the database into the container with a transaction.
        /// \tparam ContainerT Template for the container type (vector, deque, list, set or unordered_set).
        /// \param container Container to be synchronized with database content.
        /// \param mode Transaction mode.
        /// \throws sqlite_exception if an SQLite error occurs.
        template<template <class...> class ContainerT>
        void load(
                ContainerT<KeyT>& container,
                const TransactionMode& mode) {
            std::lock_guard<std::mutex> locker(m_sqlite_mutex);
            try {
                db_begin(mode);
                db_load(container);
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

        /// \brief Retrieves all keys from the database.
        /// \tparam ContainerT Template for the container type (vector, deque, list, set or unordered_set).
        /// \return A container with all keys.
        /// \throws sqlite_exception if an SQLite error occurs.
        template<template <class...> class ContainerT>
        ContainerT<KeyT> retrieve_all() {
            ContainerT<KeyT> container;
            std::unique_lock<std::mutex> locker(m_sqlite_mutex);
            db_load(container);
            locker.unlock();
            return container;
        }

        /// \brief Retrieves all keys from the database with a transaction.
        /// \tparam ContainerT Template for the container type (vector, deque, list, set or unordered_set).
        /// \param mode Transaction mode.
        /// \return A container with all keys.
        /// \throws sqlite_exception if an SQLite error occurs.
        template<template <class...> class ContainerT>
        ContainerT<KeyT> retrieve_all(const TransactionMode& mode) {
            ContainerT<KeyT> container;
            std::unique_lock<std::mutex> locker(m_sqlite_mutex);
            try {
                db_begin(mode);
                db_load(container);
                db_commit();
                locker.unlock();
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

            return container;
        }

        /// \brief Appends the content of the container to the database.
        /// \tparam ContainerT Template for the container type (vector, deque, list, set or unordered_set).
        /// \param container Container with content to be synchronized to the database.
        /// \throws sqlite_exception if an SQLite error occurs.
        template<template <class...> class ContainerT>
        void append(const ContainerT<KeyT>& container) {
            std::lock_guard<std::mutex> locker(m_sqlite_mutex);
            db_append(container);
        }

        /// \brief Appends the content of the container to the database with a transaction.
        /// \tparam ContainerT Template for the container type (vector, deque, list, set or unordered_set).
        /// \param container Container with content to be synchronized to the database.
        /// \param mode Transaction mode.
        /// \throws sqlite_exception if an SQLite error occurs.
        template<template <class...> class ContainerT>
        void append(const ContainerT<KeyT>& container, const TransactionMode& mode) {
            std::lock_guard<std::mutex> locker(m_sqlite_mutex);
            try {
                db_begin(mode);
                db_append(container);
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

        /// \brief Reconciles the database with the container.
        /// \tparam ContainerT Container type (e.g., std::set, std::unordered_set, std::vector, or std::list).
        /// \param container Container to be reconciled with the database.
        /// \throws sqlite_exception if an SQLite error occurs.
        template<template <class...> class ContainerT>
        void reconcile(const ContainerT<KeyT>& container) {
            std::lock_guard<std::mutex> locker(m_sqlite_mutex);
            db_reconcile(container);
        }

        /// \brief Reconciles the database with the container using a transaction.
        /// \tparam ContainerT Container type (e.g., std::set, std::unordered_set, std::vector, or std::list).
        /// \param container Container to be reconciled with the database.
        /// \param mode Transaction mode.
        /// \throws sqlite_exception if an SQLite error occurs.
        template<template <class...> class ContainerT>
        void reconcile(
                const ContainerT<KeyT>& container,
                const TransactionMode& mode) {
            execute_in_transaction([this, &container]() {
                db_reconcile(container);
            }, mode);
        }

        /// \brief Inserts a key into the database.
        /// \param key The key to be inserted.
        /// \throws sqlite_exception if an SQLite error occurs.
        void insert(const KeyT &key) {
            std::lock_guard<std::mutex> locker(m_sqlite_mutex);
            db_insert(key);
        }

        /// \brief Finds if a key exists in the database.
        /// \param key The key to search for.
        /// \return True if the key was found, false otherwise.
        /// \throws sqlite_exception if an SQLite error occurs.
        bool find(const KeyT &key) {
            std::lock_guard<std::mutex> locker(m_sqlite_mutex);
            return db_find(key);
        }

        /// \brief Returns the number of keys in the database.
        /// \return The number of keys in the database.
        /// \throws sqlite_exception if an SQLite error occurs.
        std::size_t count() const {
            std::lock_guard<std::mutex> locker(m_sqlite_mutex);
            return db_count();
        }

        /// \brief Checks if the database is empty (no keys present).
        /// \return True if the database is empty, false otherwise.
        /// \throws sqlite_exception if an SQLite error occurs.
        bool empty() const {
            return (db_count() == 0);
        }

        /// \brief Removes a key from the database.
        /// \param key The key to be removed.
        /// \throws sqlite_exception if an SQLite error occurs.
        void remove(const KeyT &key) {
            std::lock_guard<std::mutex> locker(m_sqlite_mutex);
            db_remove(key);
        }

        /// \brief Clears all keys from the database.
        /// \throws sqlite_exception if an SQLite error occurs.
        void clear() {
            std::lock_guard<std::mutex> locker(m_sqlite_mutex);
            db_clear();
        }

    private:
        SqliteStmt m_stmt_load;         ///< Statement for loading data from the database.
        SqliteStmt m_stmt_replace;      ///< Statement for replacing key-value pairs in the database.
        SqliteStmt m_stmt_find;         ///< Statement for finding a key.
        mutable SqliteStmt m_stmt_count;///< Statement for counting the number of keys in the database.
        SqliteStmt m_stmt_remove;       ///< Statement for removing a key.
        SqliteStmt m_stmt_clear;        ///< Statement for clearing the table.

        SqliteStmt m_stmt_insert_temp;  ///< Statement for inserting data into the temporary table.
        SqliteStmt m_stmt_purge_main;   ///< Statement for purging stale data from the main table.
        SqliteStmt m_stmt_merge_temp;   ///< Statement for merging data from the temporary table into the main table.
        SqliteStmt m_stmt_clear_temp;   ///< Statement for clearing the temporary table.

        /// \brief Creates the table in the database.
        /// \param config Configuration settings.
        void db_create_table(const Config &config) override final {
            const std::string table_name = config.table_name.empty() ? "key_store" : config.table_name;
            const std::string temp_table_name = config.table_name.empty() ? "key_temp_store" : config.table_name + "_temp";

            // Create table if they do not exist
            const std::string create_table_sql =
                "CREATE TABLE IF NOT EXISTS " + table_name + " ("
                "key " + get_sqlite_type<KeyT>() + " PRIMARY KEY NOT NULL);";
            execute(m_sqlite_db, create_table_sql);

            // Create the temporary table for synchronization if it does not exist
            const std::string create_temp_table_sql =
                "CREATE TEMPORARY TABLE IF NOT EXISTS " + temp_table_name + " ("
                "key " + get_sqlite_type<KeyT>() + " PRIMARY KEY NOT NULL);";
            execute(m_sqlite_db, create_temp_table_sql);

            // Initialize prepared statements
            m_stmt_load.init(m_sqlite_db, "SELECT key FROM " + table_name + ";");
            m_stmt_replace.init(m_sqlite_db, "REPLACE INTO " + table_name + " (key) VALUES (?);");
            m_stmt_find.init(m_sqlite_db, "SELECT EXISTS(SELECT 1 FROM " + table_name + " WHERE key = ?);");
            m_stmt_count.init(m_sqlite_db, "SELECT COUNT(*) FROM " + table_name + ";");
            m_stmt_remove.init(m_sqlite_db, "DELETE FROM " + table_name + " WHERE key = ?;");
            m_stmt_clear.init(m_sqlite_db, "DELETE FROM " + table_name);

            // Initialize prepared statements for temporary table operations
            m_stmt_insert_temp.init(m_sqlite_db, "INSERT OR REPLACE INTO " + temp_table_name + " (key) VALUES (?);");
            m_stmt_purge_main.init(m_sqlite_db, "DELETE FROM " + table_name + " WHERE key NOT IN (SELECT key FROM " + temp_table_name + ");");
            m_stmt_merge_temp.init(m_sqlite_db, "INSERT OR REPLACE INTO " + table_name + " (key) SELECT key FROM " + temp_table_name + ";");
            m_stmt_clear_temp.init(m_sqlite_db, "DELETE FROM " + temp_table_name + ";");
        }

        /// \brief Loads data from the database into the container.
        /// \tparam ContainerT Template for the container type (vector, deque, list, set or unordered_set).
        /// \param set Container to be synchronized with database content.
        /// \throws sqlite_exception if an SQLite error occurs.
        template<template <class...> class ContainerT>
        void db_load(ContainerT<KeyT>& container) {
            int err;
            try {
                for (;;) {
                    while ((err = m_stmt_load.step()) == SQLITE_ROW) {
                        KeyT key = m_stmt_load.extract_column<KeyT>(0);
                        add_value(container, key);
                    }
                    if (err == SQLITE_DONE) {
                        m_stmt_load.reset();
                        return;
                    }
                    if (err == SQLITE_BUSY) {
                        // Handle busy database, retry reading
                        m_stmt_load.reset();
                        sqlite3_sleep(SQLITE_CONTAINERS_BUSY_RETRY_DELAY_MS);
                        continue;
                    }
                    // Handle SQLite errors
                    std::string err_msg = "SQLite error: ";
                    err_msg += std::to_string(err);
                    err_msg += ", ";
                    err_msg += sqlite3_errmsg(m_sqlite_db);
                    throw sqlite_exception(err_msg, err);
                }
            } catch (...) {
                db_handle_exception(
                    std::current_exception(),
                    {&m_stmt_load},
                    "Unknown error occurred while loading data from database.");
            }
        }

        /// \brief Finds if a key exists in the database.
        /// \param key The key to search for.
        /// \return True if the key was found, false otherwise.
        /// \throws sqlite_exception if an SQLite error occurs.
        bool db_find(const KeyT& key) {
            bool is_found = false;
            int err;
            try {
                for (;;) {
                    m_stmt_find.bind_value<KeyT>(1, key);
                    while ((err = m_stmt_find.step()) == SQLITE_ROW) {
                        is_found = static_cast<bool>(m_stmt_find.extract_column<int>(0));
                    }
                    if (err == SQLITE_DONE) {
                        m_stmt_find.reset();
                        m_stmt_find.clear_bindings();
                        break;
                    }
                    if (err == SQLITE_BUSY) {
                        // Handle busy database, retry reading
                        m_stmt_find.reset();
                        m_stmt_find.clear_bindings();
                        sqlite3_sleep(SQLITE_CONTAINERS_BUSY_RETRY_DELAY_MS);
                        continue;
                    }
                    // Handle SQLite errors
                    std::string err_msg = "SQLite error: ";
                    err_msg += std::to_string(err);
                    err_msg += ", ";
                    err_msg += sqlite3_errmsg(m_sqlite_db);
                    throw sqlite_exception(err_msg, err);
                }
            } catch (...) {
                db_handle_exception(
                    std::current_exception(),
                    {&m_stmt_find},
                     "Unknown error occurred while retrieving value for the provided key.");
            }
            return is_found;
        }

        /// \brief Returns the total number of keys stored in the database.
        /// \return The number of keys stored in the database.
        /// \throws sqlite_exception if an SQLite error occurs.
        std::size_t db_count() const {
            std::size_t count = 0;
            int err;
            try {
                for (;;) {
                    while ((err = m_stmt_count.step()) == SQLITE_ROW) {
                        count = m_stmt_count.extract_column<std::size_t>(0);
                    }
                    if (err == SQLITE_DONE) {
                        m_stmt_count.reset();
                        break;
                    }
                    if (err == SQLITE_BUSY) {
                        // Handle busy database, retry reading
                        m_stmt_count.reset();
                        sqlite3_sleep(SQLITE_CONTAINERS_BUSY_RETRY_DELAY_MS);
                        continue;
                    }
                    // Handle SQLite errors
                    std::string err_msg = "SQLite error: ";
                    err_msg += std::to_string(err);
                    err_msg += ", ";
                    err_msg += sqlite3_errmsg(m_sqlite_db);
                    throw sqlite_exception(err_msg, err);
                }
            } catch (...) {
                db_handle_exception(
                    std::current_exception(),
                    {&m_stmt_count},
                    "Unknown error occurred while counting keys in the database.");
            }
            return count;
        }

        /// \brief Reconciles the content of the database with the container.
        /// Synchronizes the main table with the content of the container by using a temporary table.
        /// Clears old data, inserts new data, and updates existing records in the main table.
        /// \tparam ContainerT Template for the container type (e.g., std::set, std::unordered_set, std::vector, or std::list).
        /// \param container Container with keys to be reconciled with the database.
        /// \throws sqlite_exception if an SQLite error occurs.
        template<template <class...> class ContainerT>
        void db_reconcile(const ContainerT<KeyT>& container) {
            try {
                // Clear the temporary table
                m_stmt_clear_temp.execute();
                m_stmt_clear_temp.reset();

                // Insert all new data from the container into the temporary table
                for (const auto& item : container) {
                    m_stmt_insert_temp.bind_value<KeyT>(1, item);
                    m_stmt_insert_temp.execute();
                    m_stmt_insert_temp.reset();
                    m_stmt_insert_temp.clear_bindings();
                }
                // Remove old data from the main table that is not in the temporary table
                m_stmt_purge_main.execute();
                m_stmt_purge_main.reset();

                // Insert or update records from the temporary table into the main table
                m_stmt_merge_temp.execute();
                m_stmt_merge_temp.reset();

                // Clear the temporary table
                m_stmt_clear_temp.execute();
                m_stmt_clear_temp.reset();

            } catch (...) {
                db_handle_exception(
                    std::current_exception(), {
                        &m_stmt_purge_main, &m_stmt_merge_temp,
                        &m_stmt_clear_temp, &m_stmt_insert_temp
                    },
                    "Unknown error occurred while reconciling data.");
            }
        }

        /// \brief Appends the content of the container to the database.
        /// \tparam ContainerT Template for the container type (vector, deque, list, set or unordered_set).
        /// \param container Container with content to be synchronized to the database.
        /// \throws sqlite_exception if an SQLite error occurs.
        template<template <class...> class ContainerT>
        void db_append(const ContainerT<KeyT>& container) {
            try {
                for (const auto& item : container) {
                    m_stmt_replace.bind_value<KeyT>(1, item);
                    m_stmt_replace.execute();
                    m_stmt_replace.reset();
                    m_stmt_replace.clear_bindings();
                }
            } catch (...) {
                db_handle_exception(
                    std::current_exception(),
                    {&m_stmt_replace},
                     "Unknown error occurred while appending key-value pairs to the database.");
            }
        }

        /// \brief Inserts a key into the database.
        /// \param key The key to be inserted.
        /// \throws sqlite_exception if an SQLite error occurs.
        void db_insert(const KeyT &key) {
            try {
                m_stmt_replace.bind_value<KeyT>(1, key);
                m_stmt_replace.execute();
                m_stmt_replace.reset();
                m_stmt_replace.clear_bindings();
            } catch (...) {
                db_handle_exception(
                    std::current_exception(),
                    {&m_stmt_replace},
                     "Unknown error occurred while inserting key-value pair into the database.");
            }
        }

        /// \brief Removes a key from the database.
        /// \param key The key to be removed.
        /// \throws sqlite_exception if an SQLite error occurs.
        void db_remove(const KeyT &key) {
            try {
                m_stmt_remove.bind_value<KeyT>(1, key);
                m_stmt_remove.execute();
                m_stmt_remove.reset();
                m_stmt_remove.clear_bindings();
            } catch (...) {
                db_handle_exception(
                    std::current_exception(),
                    {&m_stmt_remove},
                    "Unknown error occurred while removing key.");
            }
        }

        /// \brief Clears all keys from the database.
        /// \throws sqlite_exception if an SQLite error occurs.
        void db_clear() {
            try {
                m_stmt_clear.execute();
                m_stmt_clear.reset();
            } catch (...) {
                db_handle_exception(
                    std::current_exception(),
                    {&m_stmt_clear},
                    "Unknown error occurred while clearing the database tables.");
            }
        }

    }; // KeyDB

}; // namespace sqlite_containers
