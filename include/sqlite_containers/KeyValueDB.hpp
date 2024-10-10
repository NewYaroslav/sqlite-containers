#pragma once

/// \file KeyValueDB.hpp
/// \brief Declaration of the KeyValueDB class for managing key-value pairs in a SQLite database.

#include "parts/BaseDB.hpp"

namespace sqlite_containers {

    /// \class KeyValueDB
    /// \brief Template class for managing key-value pairs in a SQLite database.
    /// \tparam KeyT Type of the keys.
    /// \tparam ValueT Type of the values.
    /// \details This class provides functionality to store, retrieve, and manipulate key-value pairs in a SQLite database.
    /// It supports various container types, such as `std::map`, `std::unordered_map`, `std::vector`, and `std::list`.
    /// Key-value pairs can be inserted, reconciled, retrieved, and removed with transaction support. The class includes
    /// methods for bulk loading and appending data with transactional integrity, ensuring that operations are safely executed
    /// in a database environment. Additionally, temporary tables are used during reconciliation to ensure consistent data
    /// synchronization. This class also provides methods for checking the count and emptiness of the database, and efficiently
    /// handles database errors with detailed exception handling.
    template<class KeyT, class ValueT>
    class KeyValueDB final : public BaseDB {
    public:

        /// \brief Default constructor.
        KeyValueDB() : BaseDB() {}

        /// \brief Constructor with configuration.
        /// \param config Configuration settings for the database.
        explicit KeyValueDB(const Config& config) : BaseDB() {
            set_config(config);
        }

        /// \brief Destructor.
        ~KeyValueDB() override final = default;

        // --- Operators ---

        /// \brief Assigns a container (e.g., std::map or std::unordered_map) to the database.
        /// \param container The container with key-value pairs.
        /// \return Reference to this KeyValueDB.
        /// \throws sqlite_exception if an SQLite error occurs.
        /// \note The transaction mode is taken from the database configuration.
        template<template <class...> class ContainerT>
        KeyValueDB& operator=(const ContainerT<KeyT, ValueT>& container) {
            // Get the default transaction mode from the configuration
            auto txn_mode = get_config().default_txn_mode;

            execute_in_transaction([this, &container]() {
                db_reconcile(container);
            }, txn_mode);  // Use transaction mode from the configuration
            return *this;
        }

        /// \brief Loads all key-value pairs from the database into a container (e.g., std::map or std::unordered_map).
        /// \tparam ContainerT The type of the container (e.g., std::map or std::unordered_map).
        /// \return A container populated with all key-value pairs from the database.
        /// \throws sqlite_exception if an SQLite error occurs.
        /// \note The transaction mode is taken from the database configuration.
        template<template <class...> class ContainerT = std::map>
        ContainerT<KeyT, ValueT> operator()() {
            ContainerT<KeyT, ValueT> container;
            // Get the default transaction mode from the configuration
            auto txn_mode = get_config().default_txn_mode;

            execute_in_transaction([this, &container]() {
                db_load(container);
            }, txn_mode);  // Use transaction mode from the configuration
            return container;
        }

        // --- Existing methods ---

        /// \brief Loads data from the database into the container.
        /// \tparam ContainerT Container type (e.g., std::map or std::unordered_map).
        /// \param container Container to be synchronized with database content.
        /// \throws sqlite_exception if an SQLite error occurs.
        template<template <class...> class ContainerT>
        void load(ContainerT<KeyT, ValueT>& container) {
            std::lock_guard<std::mutex> locker(m_sqlite_mutex);
            db_load(container);
        }

        /// \brief Loads data with a transaction.
        /// \tparam ContainerT Container type (e.g., std::map or std::unordered_map).
        /// \param container Container to be synchronized with database content.
        /// \param mode Transaction mode.
        /// \throws sqlite_exception if an SQLite error occurs.
        template<template <class...> class ContainerT>
        void load(
                ContainerT<KeyT, ValueT>& container,
                const TransactionMode& mode) {
            execute_in_transaction([this, &container]() {
                db_load(container);
            }, mode);
        }

        /// \brief Retrieves all key-value pairs.
        /// \tparam ContainerT Container type (e.g., std::map or std::unordered_map).
        /// \return A container with all key-value pairs.
        /// \throws sqlite_exception if an SQLite error occurs.
        template<template <class...> class ContainerT>
        ContainerT<KeyT, ValueT> retrieve_all() {
            ContainerT<KeyT, ValueT> container;
            std::lock_guard<std::mutex> locker(m_sqlite_mutex);
            db_load(container);
            return container;
        }

        /// \brief Retrieves all key-value pairs with a transaction.
        /// \tparam ContainerT Container type (e.g., std::map or std::unordered_map).
        /// \param mode Transaction mode.
        /// \return A container with all key-value pairs.
        /// \throws sqlite_exception if an SQLite error occurs.
        template<template <class...> class ContainerT>
        ContainerT<KeyT, ValueT> retrieve_all(const TransactionMode& mode) {
            ContainerT<KeyT, ValueT> container;
            execute_in_transaction([this, &container]() {
                db_load(container);
            }, mode);
            return container;
        }

        /// \brief Appends data to the database.
        /// \tparam ContainerT Container type (e.g., std::map or std::unordered_map).
        /// \param container Container with content to be synchronized.
        /// \throws sqlite_exception if an SQLite error occurs.
        template<template <class...> class ContainerT>
        void append(const ContainerT<KeyT, ValueT>& container) {
            std::lock_guard<std::mutex> locker(m_sqlite_mutex);
            db_append(container);
        }

        /// \brief Appends data with a transaction.
        /// \tparam ContainerT Container type (e.g., std::map or std::unordered_map).
        /// \param container Container with content to be synchronized.
        /// \param mode Transaction mode.
        /// \throws sqlite_exception if an SQLite error occurs.
        template<template <class...> class ContainerT>
        void append(
                const ContainerT<KeyT, ValueT>& container,
                const TransactionMode& mode) {
            execute_in_transaction([this, &container]() {
                db_append(container);
            }, mode);
        }

        /// \brief Reconciles the database with the container.
        /// \tparam ContainerT Container type (e.g., std::map or std::unordered_map).
        /// \param container Container to be reconciled with the database.
        /// \throws sqlite_exception if an SQLite error occurs.
        template<template <class...> class ContainerT>
        void reconcile(const ContainerT<KeyT, ValueT>& container) {
            std::lock_guard<std::mutex> locker(m_sqlite_mutex);
            db_reconcile(container);
        }

        /// \brief Reconciles the database with the container using a transaction.
        /// \tparam ContainerT Container type (e.g., std::map or std::unordered_map).
        /// \param container Container to be reconciled with the database.
        /// \param mode Transaction mode.
        /// \throws sqlite_exception if an SQLite error occurs.
        template<template <class...> class ContainerT>
        void reconcile(
                const ContainerT<KeyT, ValueT>& container,
                const TransactionMode& mode) {
            execute_in_transaction([this, &container]() {
                db_reconcile(container);
            }, mode);
        }

        /// \brief Inserts a key-value pair into the database.
        /// \param key The key to be inserted.
        /// \param value The value to be inserted.
        /// \throws sqlite_exception if an SQLite error occurs.
        void insert(const KeyT &key, const ValueT &value) {
            std::lock_guard<std::mutex> locker(m_sqlite_mutex);
            db_insert(key, value);
        }

        /// \brief Inserts a key-value pair into the database.
        /// \param pair The key-value pair to be inserted.
        /// \throws sqlite_exception if an SQLite error occurs.
        void insert(const std::pair<KeyT, ValueT> &pair) {
            std::lock_guard<std::mutex> locker(m_sqlite_mutex);
            db_insert(pair.first, pair.second);
        }

        /// \brief Finds a value by key.
        /// \param key The key to search for.
        /// \param value The value associated with the key.
        /// \return True if the key was found, false otherwise.
        /// \throws sqlite_exception if an SQLite error occurs.
        bool find(const KeyT &key, ValueT &value) {
            std::lock_guard<std::mutex> locker(m_sqlite_mutex);
            return db_find(key, value);
        }

        /// \brief Returns the number of elements in the database.
        /// \return The number of key-value pairs.
        /// \throws sqlite_exception if an SQLite error occurs.
        std::size_t count() const {
            std::lock_guard<std::mutex> locker(m_sqlite_mutex);
            return db_count();
        }

        /// \brief Checks if the database is empty.
        /// \return True if the database is empty, false otherwise.
        /// \throws sqlite_exception if an SQLite error occurs.
        bool empty() const {
            return (db_count() == 0);
        }

        /// \brief Removes a key-value pair from the database.
        /// \param key The key of the pair to be removed.
        /// \throws sqlite_exception if an SQLite error occurs.
        void remove(const KeyT &key) {
            std::lock_guard<std::mutex> locker(m_sqlite_mutex);
            db_remove(key);
        }

        /// \brief Clears all key-value pairs from the database.
        /// \throws sqlite_exception if an SQLite error occurs.
        void clear() {
            std::lock_guard<std::mutex> locker(m_sqlite_mutex);
            db_clear();
        }

    private:
        SqliteStmt m_stmt_load;         ///< Statement for loading data from the database.
        SqliteStmt m_stmt_replace;      ///< Statement for replacing key-value pairs in the database.
        SqliteStmt m_stmt_get_value;    ///< Statement for retrieving value by key from the database.
        mutable SqliteStmt m_stmt_count;///<
        SqliteStmt m_stmt_remove;       ///< Statement for removing key-value pair from the database.
        SqliteStmt m_stmt_clear_main;   ///< Statement for clearing the main table.

        SqliteStmt m_stmt_insert_temp;  ///< Statement for inserting data into the temporary table.
        SqliteStmt m_stmt_purge_main;   ///< Statement for purging stale data from the main table.
        SqliteStmt m_stmt_merge_temp;   ///< Statement for merging data from the temporary table into the main table.
        SqliteStmt m_stmt_clear_temp;   ///< Statement for clearing the temporary table.

        /// \brief Creates the main and temporary tables in the database.
        /// This method creates both the main key-value table and a temporary table for handling synchronization.
        /// \param config Configuration settings for the database, such as table names.
        void db_create_table(const Config &config) override final {
            const std::string table_name = config.table_name.empty() ? "kv_store" : config.table_name;
            const std::string temp_table_name = config.table_name.empty() ? "kv_temp_store" : config.table_name + "_temp";

            // Create table if they do not exist
            const std::string create_table_sql =
                "CREATE TABLE IF NOT EXISTS " + table_name + " ("
                "key " + get_sqlite_type<KeyT>() + " PRIMARY KEY NOT NULL,"
                "value " + get_sqlite_type<ValueT>() + "         NOT NULL);";
            execute(m_sqlite_db, create_table_sql);

            // Create the temporary table for synchronization if it does not exist
            const std::string create_temp_table_sql =
                "CREATE TEMPORARY TABLE IF NOT EXISTS " + temp_table_name + " ("
                "key " + get_sqlite_type<KeyT>() + " PRIMARY KEY NOT NULL,"
                "value " + get_sqlite_type<ValueT>() + "         NOT NULL);";
            execute(m_sqlite_db, create_temp_table_sql);

            // Initialize prepared statements for operations on the main table
            m_stmt_load.init(m_sqlite_db, "SELECT key, value FROM " + table_name + ";");
            m_stmt_replace.init(m_sqlite_db, "REPLACE INTO  " + table_name + " (key, value) VALUES (?, ?);");
            m_stmt_get_value.init(m_sqlite_db, "SELECT value FROM " + table_name + " WHERE key = ?;");
            m_stmt_count.init(m_sqlite_db, "SELECT COUNT(*) FROM " + table_name + ";");
            m_stmt_remove.init(m_sqlite_db, "DELETE FROM " + table_name + " WHERE key = ?;");
            m_stmt_clear_main.init(m_sqlite_db, "DELETE FROM " + table_name);

            // Initialize prepared statements for temporary table operations
            m_stmt_insert_temp.init(m_sqlite_db, "INSERT OR REPLACE INTO " + temp_table_name + " (key, value) VALUES (?, ?);");
            m_stmt_purge_main.init(m_sqlite_db, "DELETE FROM " + table_name + " WHERE key NOT IN (SELECT key FROM " + temp_table_name + ");");
            m_stmt_merge_temp.init(m_sqlite_db, "INSERT OR REPLACE INTO " + table_name + " (key, value) SELECT key, value FROM " + temp_table_name + ";");
            m_stmt_clear_temp.init(m_sqlite_db, "DELETE FROM " + temp_table_name + ";");
        }

        /// \brief Loads data from the database into the container.
        /// \tparam ContainerT Template for the container type.
        /// \param container Container to be synchronized with database content.
        /// \throws sqlite_exception if an SQLite error occurs.
        template<template <class...> class ContainerT>
        void db_load(ContainerT<KeyT, ValueT>& container) {
            int err;
            try {
                for (;;) {
                    while ((err = m_stmt_load.step()) == SQLITE_ROW) {
                        KeyT key = m_stmt_load.extract_column<KeyT>(0);
                        ValueT value = m_stmt_load.extract_column<ValueT>(1);
                        container.emplace(std::move(key), std::move(value));
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

        /// \brief Finds a value by key in the database.
        /// \param key The key to search for.
        /// \param value The value associated with the key.
        /// \return True if the key was found, false otherwise.
        /// \throws sqlite_exception if an SQLite error occurs.
        bool db_find(const KeyT& key, ValueT& value) {
            bool is_found = false;
            int err;
            try {
                for (;;) {
                    m_stmt_get_value.bind_value<KeyT>(1, key);
                    while ((err = m_stmt_get_value.step()) == SQLITE_ROW) {
                        value = m_stmt_get_value.extract_column<ValueT>(0);
                        is_found = true;
                    }
                    if (err == SQLITE_DONE) {
                        m_stmt_get_value.reset();
                        m_stmt_get_value.clear_bindings();
                        return is_found;
                    }
                    if (err == SQLITE_BUSY) {
                        // Handle busy database, retry reading
                        m_stmt_get_value.reset();
                        m_stmt_get_value.clear_bindings();
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
                    {&m_stmt_get_value},
                    "Unknown error occurred while retrieving value for the provided key.");
            }
            return false;
        }

        /// \brief Returns the number of elements in the database.
        /// \return The number of key-value pairs in the database.
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
                    "Unknown error occurred while counting key-value pairs in the database.");
            }
            return count;
        }

        /// \brief Appends the content of the container to the database.
        /// \tparam ContainerT Template for the container type (map or unordered_map).
        /// \param container Container with content to
        /// \throws sqlite_exception if an SQLite error occurs.
        template<template <class...> class ContainerT>
        void db_append(const ContainerT<KeyT, ValueT>& container) {
            try {
                for (const auto& pair : container) {
                    m_stmt_replace.bind_value<KeyT>(1, pair.first);
                    m_stmt_replace.bind_value<ValueT>(2, pair.second);
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

        /// \brief Reconciles the content of the database with the container.
        /// Synchronizes the main table with the content of the container by using a temporary table.
        /// Clears old data, inserts new data, and updates existing records in the main table.
        /// \tparam ContainerT Template for the container type (map or unordered_map).
        /// \param container Container with key-value pairs to be reconciled with the database.
        /// \throws sqlite_exception if an SQLite error occurs.
        template<template <class...> class ContainerT>
        void db_reconcile(const ContainerT<KeyT, ValueT>& container) {
            try {
                // Clear the temporary table
                m_stmt_clear_temp.execute();
                m_stmt_clear_temp.reset();

                // Insert all new data from the container into the temporary table
                for (const auto& pair : container) {
                    m_stmt_insert_temp.bind_value<KeyT>(1, pair.first);
                    m_stmt_insert_temp.bind_value<ValueT>(2, pair.second);
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

        /// \brief Inserts a key-value pair into the database.
        /// \param key The key to be inserted.
        /// \param value The value to be inserted.
        /// \throws sqlite_exception if an SQLite error occurs.
        void db_insert(const KeyT &key, const ValueT &value) {
            try {
                m_stmt_replace.bind_value<KeyT>(1, key);
                m_stmt_replace.bind_value<ValueT>(2, value);
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

        /// \brief Removes a key-value pair from the database.
        /// \param key The key of the pair to be removed.
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

        /// \brief Clears all key-value pairs from the database.
        /// \throws sqlite_exception if an SQLite error occurs.
        void db_clear() {
            try {
                m_stmt_clear_main.execute();
                m_stmt_clear_main.reset();
            } catch (...) {
                db_handle_exception(
                    std::current_exception(),
                    {&m_stmt_clear_main},
                    "Unknown error occurred while clearing the database tables.");
            }
        }

    }; // KeyValueDB

}; // namespace sqlite_containers
