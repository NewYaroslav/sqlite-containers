#pragma once

/// \file KeyMultiValueDB.hpp
/// \brief Template class for managing key-value pairs in a SQLite database.

#include "parts/BaseDB.hpp"
#include <algorithm>

namespace sqlite_containers {

    /// \class KeyMultiValueDB
    /// \brief Template class for managing key-value pairs in a SQLite database.
    ///
    /// This class allows interaction with a SQLite database where each key can map to multiple values,
    /// implementing a many-to-many relationship. It provides functionality to store, retrieve,
    /// and manipulate data using std::multimap-like semantics.
    ///
    /// \tparam KeyT Type of the keys.
    /// \tparam ValueT Type of the values.
	template<class KeyT, class ValueT>
	class KeyMultiValueDB final : public BaseDB {
	public:

		/// \brief Default constructor.
		KeyMultiValueDB() : BaseDB() {}

		/// \brief Constructor with configuration.
		/// \param config Configuration settings for the database.
		KeyMultiValueDB(const Config& config) : BaseDB() {
			set_config(config);
		}

		/// \brief Destructor.
		~KeyMultiValueDB() override final = default;

        // --- Operators ---

		/// \brief Assigns a container (e.g., std::multimap or std::unordered_multimap) to the database.
        /// \tparam ContainerT Template for the container type (e.g., std::map, std::unordered_map, std::multimap, std::unordered_multimap).
        /// \param container The container with key-value pairs to be inserted into the database.
        /// \return Reference to this KeyMultiValueDB.
        /// \throws sqlite_exception if an SQLite error occur.
        /// \note This method uses the default transaction mode from the database configuration.
        template<template <class...> class ContainerT>
        KeyMultiValueDB& operator=(const ContainerT<KeyT, ValueT>& container) {
            // Get the default transaction mode from the configuration
            auto txn_mode = get_config().default_txn_mode;

            execute_in_transaction([this, &container]() {
                db_reconcile(container);
            }, txn_mode);  // Use transaction mode from the configuration
            return *this;
        }

        /// \brief Assigns a container where each key maps to a collection of values to the database.
        /// \tparam ContainerT Template for the outer container type (e.g., std::map, std::unordered_map, std::multimap, std::unordered_multimap).
        /// \tparam ValueContainerT Template for the inner container type used for values (e.g., std::vector, std::set).
        /// \param container The container where each key maps to a collection of values.
        /// \return Reference to this KeyMultiValueDB.
        /// \throws sqlite_exception if an SQLite error occurs.
        /// \note This method uses the default transaction mode from the database configuration.
        template<template <class...> class ContainerT, template <class...> class ValueContainerT>
        KeyMultiValueDB& operator=(const ContainerT<KeyT, ValueContainerT<ValueT>>& container) {
            // Get the default transaction mode from the configuration
            auto txn_mode = get_config().default_txn_mode;

            execute_in_transaction([this, &container]() {
                db_reconcile(container);
            }, txn_mode);  // Use transaction mode from the configuration
            return *this;
        }

        /// \brief Loads all key-value pairs from the database into a container.
        /// \tparam ContainerT The type of the container (e.g., std::map, std::unordered_map, std::multimap, std::unordered_multimap).
        /// \return A container populated with all key-value pairs from the database.
        /// \throws sqlite_exception if an SQLite error occurs.
        /// \note This method uses the default transaction mode from the database configuration.
        template<template <class...> class ContainerT = std::multimap>
        ContainerT<KeyT, ValueT> operator()() {
            ContainerT<KeyT, ValueT> container;
            // Get the default transaction mode from the configuration
            auto txn_mode = get_config().default_txn_mode;

            execute_in_transaction([this, &container]() {
                db_load(container);
            }, txn_mode);  // Use transaction mode from the configuration
            return container;
        }

        /// \brief Loads all key-value pairs, where each key maps to a collection of values, from the database.
        /// \tparam ContainerT Template for the outer container type (e.g., std::map, std::unordered_map, std::multimap, std::unordered_multimap).
        /// \tparam ValueContainerT Template for the inner container type used for values (e.g., std::vector, std::set).
        /// \return A container populated with all key-value pairs, where each key maps to a collection of values.
        /// \throws sqlite_exception if an SQLite error occurs.
        /// \note This method uses the default transaction mode from the database configuration.
        template<template <class...> class ContainerT, template <class...> class ValueContainerT>
        ContainerT<KeyT, ValueContainerT<ValueT>> operator()() {
            ContainerT<KeyT, ValueContainerT<ValueT>> container;
            // Get the default transaction mode from the configuration
            auto txn_mode = get_config().default_txn_mode;

            execute_in_transaction([this, &container]() {
                db_load(container);
            }, txn_mode);  // Use transaction mode from the configuration
            return container;
        }

        // --- Existing methods ---

		/// \brief Loads data from the database into the container.
        /// \tparam ContainerT Template for the container type (e.g., std::map, std::unordered_map, std::multimap, std::unordered_multimap).
        /// \param container Container to load the data into.
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

		/// \brief Loads data from the database into the container.
        /// \tparam ContainerT Template for the container type (e.g., std::map, std::unordered_map, std::multimap, std::unordered_multimap).
        /// \param container Container to load the data into.
        /// \throws sqlite_exception if an SQLite error occurs.
		template<template <class...> class ContainerT>
		void load(
				ContainerT<KeyT, ValueT>& container) {
            std::lock_guard<std::mutex> locker(m_sqlite_mutex);
            db_load(container);
		}

		/// \brief Loads data from the database into the container.
		/// \tparam ContainerT Template for the container type (std::map, std::unordered_map, std::multimap or std::unordered_multimap).
        /// \tparam ValueContainerT Template for the container type used for values (e.g., std::vector, std::set).
		/// \param container Container to load the data into.
		/// \param mode Transaction mode.
		/// \throws sqlite_exception if an SQLite error occurs.
		template<template <class...> class ContainerT, template <class...> class ValueContainerT>
		void load(
				ContainerT<KeyT, ValueContainerT<ValueT>>& container,
				const TransactionMode& mode) {
			execute_in_transaction([this, &container]() {
                db_load(container);
            }, mode);
		}

        /// \brief Loads data from the database into the container.
		/// \tparam ContainerT Template for the container type (std::map, std::unordered_map, std::multimap or std::unordered_multimap).
        /// \tparam ValueContainerT Template for the container type used for values (e.g., std::vector, std::set).
		/// \param container Container to load the data into.
		/// \throws sqlite_exception if an SQLite error occurs.
		template<template <class...> class ContainerT, template <class...> class ValueContainerT>
		void load(
				ContainerT<KeyT, ValueContainerT<ValueT>>& container) {
            std::lock_guard<std::mutex> locker(m_sqlite_mutex);
			db_load(container);
		}

		/// \brief Retrieves all key-value pairs from the database with a transaction.
        /// \tparam ContainerT Template for the container type (std::map, std::unordered_map, std::multimap or std::unordered_multimap).
        /// \param mode Transaction mode.
        /// \return A container with all key-value pairs.
        /// \throws sqlite_exception if an SQLite error occurs.
		template<template <class...> class ContainerT>
		ContainerT<KeyT, ValueT> retrieve_all(
                const TransactionMode& mode) {
			ContainerT<KeyT, ValueT> container;
			execute_in_transaction([this, &container]() {
                db_load(container);
            }, mode);
            return container;
		}

		/// \brief Retrieves all key-value pairs from the database.
        /// \tparam ContainerT Template for the container type (std::map, std::unordered_map, std::multimap or std::unordered_multimap).
        /// \return A container with all key-value pairs.
        /// \throws sqlite_exception if an SQLite error occurs.
		template<template <class...> class ContainerT>
		ContainerT<KeyT, ValueT> retrieve_all() {
			ContainerT<KeyT, ValueT> container;
			std::unique_lock<std::mutex> locker(m_sqlite_mutex);
			db_load(container);
			locker.unlock();
            return container;
		}

        /// \brief Retrieves all key-value pairs from the database with a transaction.
        /// \tparam ContainerT Template for the container type (std::map, std::unordered_map, std::multimap or std::unordered_multimap).
        /// \tparam ValueContainerT Template for the container type used for values (e.g., std::vector, std::set).
        /// \param mode Transaction mode.
        /// \return A container with all key-value pairs.
        /// \throws sqlite_exception if an SQLite error occurs.
		template<template <class...> class ContainerT, template <class...> class ValueContainerT>
		ContainerT<KeyT, ValueContainerT<ValueT>> retrieve_all(
                const TransactionMode& mode) {
			ContainerT<KeyT, ValueContainerT<ValueT>> container;
			execute_in_transaction([this, &container]() {
                db_load(container);
            }, mode);
            return container;
		}

        /// \brief Retrieves all key-value pairs from the database.
        /// \tparam ContainerT Template for the container type (std::map, std::unordered_map, std::multimap or std::unordered_multimap).
        /// \tparam ValueContainerT Template for the container type used for values (e.g., std::vector, std::set).
        /// \return A container with all key-value pairs.
        /// \throws sqlite_exception if an SQLite error occurs.
		template<template <class...> class ContainerT, template <class...> class ValueContainerT>
		ContainerT<KeyT, ValueContainerT<ValueT>> retrieve_all() {
			ContainerT<KeyT, ValueContainerT<ValueT>> container;
			std::unique_lock<std::mutex> locker(m_sqlite_mutex);
			db_load(container);
			locker.unlock();
            return container;
		}

        /// \brief Appends the content of the container to the database with a transaction.
        /// \tparam ContainerT Template for the container type (std::map, std::unordered_map, std::multimap or std::unordered_multimap).
        /// \param container Container with content to be appended to the database.
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

		/// \brief Appends the content of the container to the database.
        /// \tparam ContainerT Template for the container type (std::map, std::unordered_map, std::multimap or std::unordered_multimap).
        /// \param container Container with content to be appended to the database.
        /// \throws sqlite_exception if an SQLite error occurs.
		template<template <class...> class ContainerT>
		void append(
                const ContainerT<KeyT, ValueT>& container) {
			std::lock_guard<std::mutex> locker(m_sqlite_mutex);
			db_append(container);
		}

        /// \brief Appends the content of the container to the database with a transaction.
        /// \tparam ContainerT Template for the container type (std::map, std::unordered_map, std::multimap or std::unordered_multimap).
        /// \tparam ValueContainerT Template for the container type used for values (e.g., std::vector, std::set).
        /// \param container Container with content to be appended to the database.
        /// \param mode Transaction mode.
        /// \throws sqlite_exception if an SQLite error occurs.
		template<template <class...> class ContainerT, template <class...> class ValueContainerT>
		void append(
                const ContainerT<KeyT, ValueContainerT<ValueT>>& container,
                const TransactionMode& mode) {
			execute_in_transaction([this, &container]() {
                db_append(container);
            }, mode);
		}

		/// \brief Appends the content of the container to the database.
        /// \tparam ContainerT Template for the container type (std::map, std::unordered_map, std::multimap or std::unordered_multimap).
        /// \tparam ValueContainerT Template for the container type used for values (e.g., std::vector, std::set).
        /// \param container Container with content to be appended to the database.
        /// \throws sqlite_exception if an SQLite error occurs.
		template<template <class...> class ContainerT, template <class...> class ValueContainerT>
		void append(
                const ContainerT<KeyT, ValueContainerT<ValueT>>& container) {
			std::lock_guard<std::mutex> locker(m_sqlite_mutex);
			db_append(container);
		}

        /// \brief Reconciles the content of the container with the database with a transaction.
        /// \tparam ContainerT Template for the container type (std::map, std::unordered_map, std::multimap or std::unordered_multimap).
        /// \param container Container with content to be reconciled with the database.
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

		/// \brief Reconciles the content of the container with the database.
        /// \tparam ContainerT Template for the container type (std::map, std::unordered_map, std::multimap or std::unordered_multimap).
        /// \param container Container with content to be reconciled with the database.
        /// \throws sqlite_exception if an SQLite error occurs.
        template<template <class...> class ContainerT>
		void reconcile(
                const ContainerT<KeyT, ValueT>& container) {
            std::lock_guard<std::mutex> locker(m_sqlite_mutex);
            db_reconcile(container);
		}

		/// \brief Reconciles the content of the container with the database with a transaction.
        /// \tparam ContainerT Template for the container type (map or unordered_map).
        /// \tparam ValueContainerT Template for the container type used for values.
        /// \param container Container with content to be reconciled with the database.
        /// \param mode Transaction mode.
        /// \throws sqlite_exception if an SQLite error occurs.
        template<template <class...> class ContainerT, template <class...> class ValueContainerT>
		void reconcile(
                const ContainerT<KeyT, ValueContainerT<ValueT>>& container,
                const TransactionMode& mode) {
            execute_in_transaction([this, &container]() {
                db_reconcile(container);
            }, mode);
		}

		/// \brief Reconciles the content of the container with the database.
        /// \tparam ContainerT Template for the container type (map or unordered_map).
        /// \tparam ValueContainerT Template for the container type used for values.
        /// \param container Container with content to be reconciled with the database.
        /// \throws sqlite_exception if an SQLite error occurs.
        template<template <class...> class ContainerT, template <class...> class ValueContainerT>
		void reconcile(const ContainerT<KeyT, ValueContainerT<ValueT>>& container) {
            std::lock_guard<std::mutex> locker(m_sqlite_mutex);
            db_reconcile(container);
		}

        /// \brief Inserts a key-value pair into the database with a transaction.
        /// \param key The key to be inserted.
        /// \param value The value to be inserted.
        /// \param mode Transaction mode (defaults to the mode from the configuration).
        /// \throws sqlite_exception if an SQLite error occurs.
        void insert(
                const KeyT &key,
                const ValueT &value,
                const TransactionMode& mode) {
            execute_in_transaction([this, &key, &value]() {
                db_insert(key, value);
            }, mode);
        }

        /// \brief Inserts a key-value pair into the database.
        /// \param key The key to be inserted.
        /// \param value The value to be inserted.
        /// \param mode Transaction mode (defaults to the mode from the configuration).
        /// \throws sqlite_exception if an SQLite error occurs.
        void insert(
                const KeyT &key,
                const ValueT &value) {
            std::lock_guard<std::mutex> locker(m_sqlite_mutex);
            db_insert(key, value);
        }

        /// \brief Inserts a key-value pair into the database with a transaction.
        /// \param pair The key-value pair to be inserted.
        /// \param mode Transaction mode (defaults to the mode from the configuration).
        /// \throws sqlite_exception if an SQLite error occurs.
        void insert(
                const std::pair<KeyT, ValueT> &pair,
                const TransactionMode& mode) {
			execute_in_transaction([this, &pair]() {
                db_insert(pair.first, pair.second);
            }, mode);
		}

		/// \brief Inserts a key-value pair into the database.
        /// \param pair The key-value pair to be inserted.
        /// \throws sqlite_exception if an SQLite error occurs.
        void insert(
                const std::pair<KeyT, ValueT> &pair) {
			std::lock_guard<std::mutex> locker(m_sqlite_mutex);
			db_insert(pair.first, pair.second);
		}

        /// \brief Sets the count of values associated with a specific key-value pair in the database.
        /// \param key The key of the pair.
        /// \param value The value of the pair.
        /// \param value_count The count to set.
        /// \throws sqlite_exception if an SQLite error occurs.
		void set_value_count(
                const KeyT& key,
                const ValueT& value,
                const std::size_t& value_count) {
            std::lock_guard<std::mutex> locker(m_sqlite_mutex);
            db_set_value_count_key_value(key, value, value_count);
        }

        /// \brief Alias for set_value_count().
        /// This method is an alias for set_value_count() and performs the same operation.
        /// \param key The key of the pair.
        /// \param value The value of the pair.
        /// \param value_count The count to set.
        /// \throws sqlite_exception if an SQLite error occurs.
        void set_count(
                const KeyT& key,
                const ValueT& value,
                const std::size_t& value_count) {
            std::lock_guard<std::mutex> locker(m_sqlite_mutex);
            db_set_value_count_key_value(key, value, value_count);
        }

        /// \brief Retrieves the count of values associated with a specific key-value pair from the database.
        /// This method returns the number of associations between a given key and value in the database.
        /// \param key The key of the pair.
        /// \param value The value of the pair.
        /// \return The count of values associated with the key-value pair.
        /// \throws sqlite_exception if an SQLite error occurs.
        std::size_t get_value_count(const KeyT& key, const ValueT& value) const {
            std::lock_guard<std::mutex> locker(m_sqlite_mutex);
            return db_get_value_count_key_value(key, value);
        }

        /// \brief Alias for get_value_count().
        /// This method is an alias for get_value_count() and performs the same operation.
        /// \param key The key of the pair.
        /// \param value The value of the pair.
        /// \return The count of values associated with the key-value pair.
        /// \throws sqlite_exception if an SQLite error occurs.
        std::size_t count(const KeyT& key, const ValueT& value) const {
            std::lock_guard<std::mutex> locker(m_sqlite_mutex);
            return db_get_value_count_key_value(key, value);
		}

        /// \brief Finds values by key in the database.
        /// \tparam ContainerT Template for the container type.
        /// \param key The key to search for.
        /// \param values The container to store the values associated with the key.
        /// \return True if the key was found, false otherwise.
        /// \throws sqlite_exception if an SQLite error occurs.
        template<template <class...> class ContainerT>
        bool find(const KeyT &key, ContainerT<ValueT>& values) {
            std::lock_guard<std::mutex> locker(m_sqlite_mutex);
            return db_find(key, values);
        }

        /// \brief Returns the number of elements in the database.
        /// This method returns the number of unique keys stored in the database, not the number of key-value pairs.
		/// \return The number of key-value pairs.
		/// \throws sqlite_exception if an SQLite error occurs.
		std::size_t count() const {
            std::lock_guard<std::mutex> locker(m_sqlite_mutex);
            return db_count_key();
		}

        /// \brief Checks if the database is empty.
        /// This method checks if there are any keys stored in the database.
		/// \return True if the database is empty, false otherwise.
		/// \throws sqlite_exception if an SQLite error occurs.
		bool empty() const {
            return (db_count_key() == 0);
		}

        /// \brief Removes a specific key-value pair from the database.
        /// \param key The key of the pair to be removed.
        /// \param value The value of the pair to be removed.
        /// \throws sqlite_exception if an SQLite error occurs.
        void remove(const KeyT &key, const ValueT &value) {
            std::lock_guard<std::mutex> locker(m_sqlite_mutex);
            db_remove_key_value(key, value);
        }

        /// \brief Removes all values associated with a key from the database.
        /// \param key The key of the pairs to be removed.
        /// \throws sqlite_exception if an SQLite error occurs.
        void remove(const KeyT &key) {
            std::lock_guard<std::mutex> locker(m_sqlite_mutex);
            db_remove_all_values(key);
        }

        /// \brief Clears all key-value pairs from the database with a transaction.
        /// \param mode Transaction mode (defaults to the mode from the configuration).
        /// \throws sqlite_exception if an SQLite error occurs.
        void clear(const TransactionMode& mode) {
            execute_in_transaction([this]() {
                db_clear();
            }, mode);
        }

        /// \brief Clears all key-value pairs from the database.
        /// \param mode Transaction mode (defaults to the mode from the configuration).
        /// \throws sqlite_exception if an SQLite error occurs.
        void clear() {
            std::lock_guard<std::mutex> locker(m_sqlite_mutex);
            db_clear();
        }

	private:

        // Statements for loading and managing keys, values, and key-value pairs
        SqliteStmt m_stmt_load;                    ///< Statement for loading data from the database.

        SqliteStmt m_stmt_insert_key;              ///< Statement for inserting a key.
        SqliteStmt m_stmt_insert_value;            ///< Statement for inserting a value.
        SqliteStmt m_stmt_insert_key_value;        ///< Statement for inserting a key-value pair.

        SqliteStmt m_stmt_get_key_id;              ///< Statement for retrieving key ID.
        SqliteStmt m_stmt_get_value_id;            ///< Statement for retrieving value ID.
        SqliteStmt m_stmt_get_value_count;         ///< Statement for retrieving the count of a value.
        mutable SqliteStmt m_stmt_get_value_count_kv; ///< Statement for retrieving the count of a value by key-value pair.

        mutable SqliteStmt m_stmt_count_key;       ///< Statement for counting the number of keys.

        // Statements for temporary tables
        SqliteStmt m_stmt_insert_key_temp;         ///< Statement for inserting keys into the temporary table.
        SqliteStmt m_stmt_insert_value_temp;       ///< Statement for inserting values into the temporary table.

        // Statements for purging old keys and values
        SqliteStmt m_stmt_purge_keys;              ///< Statement for purging old keys.
        SqliteStmt m_stmt_purge_values;            ///< Statement for purging old values.

        // Statements for clearing temporary tables
        SqliteStmt m_stmt_clear_keys_temp;         ///< Statement for clearing the temporary keys table.
        SqliteStmt m_stmt_clear_values_temp;       ///< Statement for clearing the temporary values table.

        // Statements for managing value counts
        SqliteStmt m_stmt_set_value_count;         ///< Statement for setting value count for a key-value pair.
        SqliteStmt m_stmt_set_value_count_kv;      ///< Statement for setting value count by key-value pair.

        SqliteStmt m_stmt_find;                    ///< Statement for finding values by key.

        // Statements for removing key-value pairs and clearing tables
        SqliteStmt m_stmt_remove_key_value;        ///< Statement for removing a specific key-value pair.
        SqliteStmt m_stmt_remove_all_values;       ///< Statement for removing all values associated with a key.

        SqliteStmt m_stmt_clear_keys;              ///< Statement for clearing the keys table.
        SqliteStmt m_stmt_clear_values;            ///< Statement for clearing the values table.
        SqliteStmt m_stmt_clear_key_values;        ///< Statement for clearing the key-value pairs table.

		/// \brief Creates the tables in the database.
        /// This method creates both the main and temporary tables for keys, values, and key-value pairs.
        /// \param config Configuration settings for the database, such as table names.
		void db_create_table(const Config &config) override final {
			const std::string keys_table = config.table_name.empty() ? "keys_store" : config.table_name + "_keys";
			const std::string values_table = config.table_name.empty() ? "values_store" : config.table_name + "_values";
			const std::string key_value_table = config.table_name.empty() ? "key_value_store" : config.table_name + "_key_value";

			const std::string keys_temp_table = config.table_name.empty() ? "keys_temp_store" : config.table_name + "_temp_keys";
			const std::string values_temp_table = config.table_name.empty() ? "values_temp_store" : config.table_name + "_temp_values";

			// Create main tables if they do not exist
			const std::string create_keys_table_sql =
                "CREATE TABLE IF NOT EXISTS " + keys_table + " ("
                "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                "key " + get_sqlite_type<KeyT>() + " NOT NULL UNIQUE);";
            execute(m_sqlite_db, create_keys_table_sql);

            const std::string create_values_table_sql =
                "CREATE TABLE IF NOT EXISTS " + values_table + " ("
                "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                "value " + get_sqlite_type<ValueT>() + " NOT NULL UNIQUE);";
            execute(m_sqlite_db, create_values_table_sql);

            const std::string create_key_value_table_sql =
                "CREATE TABLE IF NOT EXISTS " + key_value_table + " ("
                "key_id INTEGER NOT NULL, "
                "value_id INTEGER NOT NULL, "
                "value_count INTEGER DEFAULT 1, "
                "FOREIGN KEY(key_id) REFERENCES " + keys_table + "(id) ON DELETE CASCADE, "
                "FOREIGN KEY(value_id) REFERENCES " + values_table + "(id) ON DELETE CASCADE, "
                "PRIMARY KEY (key_id, value_id));";
            execute(m_sqlite_db, create_key_value_table_sql);

            // Create temporary tables for synchronization
            const std::string create_keys_temp_table_sql =
                "CREATE TEMPORARY TABLE IF NOT EXISTS " + keys_temp_table + " ("
                "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                "key " + get_sqlite_type<KeyT>() + " NOT NULL UNIQUE);";
            execute(m_sqlite_db, create_keys_temp_table_sql);

            const std::string create_values_temp_table_sql =
                "CREATE TEMPORARY TABLE IF NOT EXISTS " + values_temp_table + " ("
                "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                "value " + get_sqlite_type<ValueT>() + " NOT NULL UNIQUE);";
            execute(m_sqlite_db, create_values_temp_table_sql);

            // Enable foreign keys
            execute(m_sqlite_db, "PRAGMA foreign_keys = ON;");

            // Initialize prepared statements for main tables
            m_stmt_load.init(m_sqlite_db,
                "SELECT " + keys_table + ".key, " + values_table + ".value, " + key_value_table + ".value_count "
                "FROM " + keys_table + " "
                "JOIN " + key_value_table + " ON " + keys_table + ".id = " + key_value_table + ".key_id "
                "JOIN " + values_table + " ON " + key_value_table + ".value_id = " + values_table + ".id;");

            m_stmt_insert_key.init(m_sqlite_db, "INSERT OR IGNORE INTO " + keys_table + " (key) VALUES (?);");
            m_stmt_insert_value.init(m_sqlite_db, "INSERT OR IGNORE INTO " + values_table + " (value) VALUES (?);");
            m_stmt_insert_key_value.init(m_sqlite_db, "INSERT INTO " + key_value_table + " (key_id, value_id) VALUES (?, ?);");

            m_stmt_get_key_id.init(m_sqlite_db, "SELECT id FROM " + keys_table + " WHERE key = ?;");
            m_stmt_get_value_id.init(m_sqlite_db, "SELECT id FROM " + values_table + " WHERE value = ?;");
            m_stmt_get_value_count.init(m_sqlite_db, "SELECT value_count FROM " + key_value_table + " WHERE key_id = ? AND value_id = ?;");
            m_stmt_get_value_count_kv.init(m_sqlite_db,
                "SELECT value_count FROM " + key_value_table +
                " WHERE key_id = (SELECT id FROM " + keys_table +
                " WHERE key = ?) AND value_id = (SELECT id FROM " + values_table +
                " WHERE value = ?);");

            m_stmt_count_key.init(m_sqlite_db, "SELECT COUNT(*) FROM " + keys_table + ";");

            // Initialize prepared statements for temporary tables
            m_stmt_insert_key_temp.init(m_sqlite_db, "INSERT OR IGNORE INTO " + keys_temp_table + " (key) VALUES (?);");
            m_stmt_insert_value_temp.init(m_sqlite_db, "INSERT OR IGNORE INTO " + values_temp_table + " (value) VALUES (?);");

            // Statements for purging old data and clearing temporary tables
            m_stmt_purge_keys.init(m_sqlite_db, "DELETE FROM " + keys_table + " WHERE key NOT IN (SELECT key FROM " + keys_temp_table + ");");
            m_stmt_purge_values.init(m_sqlite_db, "DELETE FROM " + values_table + " WHERE value NOT IN (SELECT value FROM " + values_temp_table + ");");

            m_stmt_clear_keys_temp.init(m_sqlite_db, "DELETE FROM " + keys_temp_table + ";");
            m_stmt_clear_values_temp.init(m_sqlite_db, "DELETE FROM " + values_temp_table + ";");

            // Statements for setting value counts
            m_stmt_set_value_count.init(m_sqlite_db, "UPDATE " + key_value_table + " SET value_count = ? WHERE key_id = ? AND value_id = ?;");
            m_stmt_set_value_count_kv.init(m_sqlite_db,
                "UPDATE " + key_value_table +
                " SET value_count = ? WHERE key_id = (SELECT id FROM " + keys_table +
                " WHERE key = ?) AND value_id = (SELECT id FROM " + values_table +
                " WHERE value = ?);");

            // Statement for finding key-value pairs
            m_stmt_find.init(m_sqlite_db,
                "SELECT v.value, kv.value_count "
                "FROM " + values_table + " v "
                "JOIN " + key_value_table + " kv ON v.id = kv.value_id "
                "JOIN " + keys_table + " k ON kv.key_id = k.id "
                "WHERE k.key = ?;");

            // Statements for removing key-value pairs and clearing tables
            m_stmt_remove_key_value.init(m_sqlite_db, "DELETE FROM " + key_value_table + " WHERE key_id = (SELECT id FROM " + keys_table + " WHERE key = ?) AND value_id = (SELECT id FROM " + values_table + " WHERE value = ?);");
            m_stmt_remove_all_values.init(m_sqlite_db, "DELETE FROM " + keys_table + " WHERE key = ?");

            m_stmt_clear_keys.init(m_sqlite_db, "DELETE FROM " + keys_table + ";");
            m_stmt_clear_values.init(m_sqlite_db, "DELETE FROM " + values_table + ";");
            m_stmt_clear_key_values.init(m_sqlite_db, "DELETE FROM " + key_value_table + ";");
		}

		/// \brief Loads data from the database into the container.
		/// \tparam ContainerT Template for the container type.
		/// \param container Container to load the data into.
		/// \throws sqlite_exception if an SQLite error occurs.
		template<template <class...> class ContainerT>
		void db_load(ContainerT<KeyT, ValueT>& container) {
			int err;
			try {
				for (;;) {
					while ((err = m_stmt_load.step()) == SQLITE_ROW) {
						KeyT key = m_stmt_load.extract_column<KeyT>(0);
						ValueT value = m_stmt_load.extract_column<ValueT>(1);
						size_t value_count = m_stmt_load.extract_column<size_t>(2);
						add_value(container, key, value, value_count);
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
			} catch(const sqlite_exception &e) {
				m_stmt_load.reset();
				throw e;
			} catch(const std::exception &e) {
				m_stmt_load.reset();
				throw sqlite_exception(e.what());
			} catch(...) {
				m_stmt_load.reset();
				throw sqlite_exception("Unknown error occurred while loading data from database.");
			}
		}

		/// \brief Loads data from the database into the container.
        /// \tparam ContainerT Template for the map container type.
        /// \tparam ValueContainerT Template for the container type used for values.
        /// \param container Container to load the data into.
        /// \throws sqlite_exception if an SQLite error occurs.
        template<template <class...> class ContainerT, template <class...> class ValueContainerT>
        void db_load(ContainerT<KeyT, ValueContainerT<ValueT>>& container) {
            int err;
            try {
                for (;;) {
                    while ((err = m_stmt_load.step()) == SQLITE_ROW) {
                        KeyT key = m_stmt_load.extract_column<KeyT>(0);
                        ValueT value = m_stmt_load.extract_column<ValueT>(1);
                        const size_t value_count = m_stmt_load.extract_column<size_t>(2);
                        add_value(container[key], value, value_count);
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
            } catch(const sqlite_exception &e) {
                m_stmt_load.reset();
                throw e;
            } catch(const std::exception &e) {
                m_stmt_load.reset();
                throw sqlite_exception(e.what());
            } catch(...) {
                m_stmt_load.reset();
                throw sqlite_exception("Unknown error occurred while loading data from database.");
            }
        }

        /// \brief Appends the content of the container to the database.
        /// \tparam ContainerT Template for the container type.
        /// \param container Container with content to be appended to the database.
        /// \throws sqlite_exception if an SQLite error occurs.
		template<template <class...> class ContainerT>
		void db_append(const ContainerT<KeyT, ValueT>& container) {
			try {
				for (const auto& pair : container) {
                    // Insert the key if it doesn't already exist
                    db_insert_key(pair.first);
                    // Insert the value if it doesn't already exist
                    db_insert_value(pair.second);
                    // Get the key_id
                    const int64_t key_id = db_get_key_id(pair.first);
                    if (key_id == -1) continue;
                    // Get the value_id
                    const int64_t value_id = db_get_value_id(pair.second);
                    if (value_id == -1) continue;
                    // Get the value_count
                    const size_t value_count = db_get_value_count(key_id, value_id);
                    if (value_count) {
                        // Update the value_count with the key_id and value_id
                        db_set_value_count(key_id, value_id, value_count + 1);
                    } else {
                        // Insert the key-value pair
                        db_insert_key_value(key_id, value_id);
                    }
				}
			} catch (const sqlite_exception &e) {
                db_handle_insert_exception(e);
            } catch (const std::exception &e) {
                db_handle_insert_exception(sqlite_exception(e.what()));
            } catch (...) {
                db_handle_insert_exception(sqlite_exception("Unknown error occurred while inserting key-value pair."));
            }
		}

        /// \brief Appends the content of the container to the database.
        /// \tparam ContainerT Template for the container type.
        /// \tparam ValueContainerT Template for the container type used for values.
        /// \param container Container with content to be appended to the database.
        /// \throws sqlite_exception if an SQLite error occurs.
		template<template <class...> class ContainerT, template <class...> class ValueContainerT>
		void db_append(
                const ContainerT<KeyT, ValueContainerT<ValueT>>& container) {
			try {
				for (const auto& pair : container) {
                    // Insert the key if it doesn't already exist
                    db_insert_key(pair.first);
                    // Get the key_id
                    const int64_t key_id = db_get_key_id(pair.first);
                    if (key_id == -1) continue;
                    for (const auto& value : pair.second) {
                        // Insert the value if it doesn't already exist
                        db_insert_value(value);
                        // Get the value_id
                        const int64_t value_id = db_get_value_id(value);
                        if (value_id == -1) continue;
                        // Get the value_count
                        const size_t value_count = db_get_value_count(key_id, value_id);
                        if (value_count) {
                            // Update the value_count with the key_id and value_id
                            db_set_value_count(key_id, value_id, value_count + 1);
                        } else {
                            // Insert the key-value pair
                            db_insert_key_value(key_id, value_id);
                        }
                    }
				}
			} catch (const sqlite_exception &e) {
                db_handle_insert_exception(e);
            } catch (const std::exception &e) {
                db_handle_insert_exception(sqlite_exception(e.what()));
            } catch (...) {
                db_handle_insert_exception(sqlite_exception("Unknown error occurred while inserting key-value pair."));
            }
		}

        /// \brief Inserts a key-value pair into the database.
        /// \param key The key to be inserted.
        /// \param value The value to be inserted.
        /// \throws sqlite_exception if an SQLite error occurs.
		void db_insert(
                const KeyT &key,
                const ValueT &value) {
            try {
                // Insert the key if it doesn't already exist
                db_insert_key(key);
                // Insert the value if it doesn't already exist
                db_insert_value(value);
                // Get the key_id
                const int64_t key_id = db_get_key_id(key);
                if (key_id == -1) throw sqlite_exception("Key ID not found.");
                // Get the value_id
                const int64_t value_id = db_get_value_id(value);
                if (value_id == -1) throw sqlite_exception("Value ID not found.");
                // Get the value_count
                const size_t value_count = db_get_value_count(key_id, value_id);
                if (value_count) {
                    // Update the value_count with the key_id and value_id
                    db_set_value_count(key_id, value_id, value_count + 1);
                } else {
                    // Insert the key-value pair
                    db_insert_key_value(key_id, value_id);
                }
            } catch (const sqlite_exception &e) {
                db_handle_insert_exception(e);
            } catch (const std::exception &e) {
                db_handle_insert_exception(sqlite_exception(e.what()));
            } catch (...) {
                db_handle_insert_exception(sqlite_exception("Unknown error occurred while inserting key-value pair."));
            }
        }

        /// \brief Handles an exception that occurs during insertion.
        /// This method resets the prepared statements and clears bindings when an exception is encountered during the insertion process.
        /// \param e The exception that occurred.
        /// \throws sqlite_exception The original exception is rethrown after handling.
        void db_handle_insert_exception(const sqlite_exception &e) {
            m_stmt_insert_key.reset();
            m_stmt_insert_key.clear_bindings();
            m_stmt_insert_value.reset();
            m_stmt_insert_value.clear_bindings();
            m_stmt_get_key_id.reset();
            m_stmt_get_key_id.clear_bindings();
            m_stmt_get_value_id.reset();
            m_stmt_get_value_id.clear_bindings();
            m_stmt_get_value_count.reset();
            m_stmt_get_value_count.clear_bindings();
            m_stmt_set_value_count.reset();
            m_stmt_set_value_count.clear_bindings();
            throw e;
		}

        /// \brief Reconciles the database with the provided container.
        /// This method compares the content of the provided container with the database,
        /// ensuring that all key-value pairs are synchronized. If keys or values do not exist in the database,
        /// they are inserted. If old data exists in the database, it is removed.
        /// \tparam ContainerT Template for the container type (e.g., std::map, std::unordered_map).
        /// \param container The container with key-value pairs to reconcile with the database.
        /// \throws sqlite_exception if an SQLite error occurs.
		template<template <class...> class ContainerT>
		void db_reconcile(
                const ContainerT<KeyT, ValueT>& container) {
            try {
                // Collect value repetitions
				std::unordered_map<KeyT, std::vector<std::pair<ValueT, int>>> temp_container;
                for (const auto& pair : container) {
                    auto& vec = temp_container[pair.first];
                    auto it = find_or_insert(vec, pair.second);
                    it->second++;
                }

				db_clear_temp_tables();
                for (const auto& item : temp_container) {
                    db_insert_key(item.first);
                    db_insert_key_temp(item.first);
                    for (const auto& pair : item.second) {
                        db_insert_value(pair.first);
                        db_insert_value_temp(pair.first);
                    }
                }
                for (const auto& pair : container) {
                    const int64_t key_id = db_get_key_id(pair.first);
                    if (key_id == -1) throw sqlite_exception("Failed to retrieve key ID for the provided key during reconciliation.");
                    const int64_t value_id = db_get_value_id(pair.second);
                    if (value_id == -1) throw sqlite_exception("Failed to retrieve value ID for the provided value during reconciliation.");
                    const size_t value_count = db_get_value_count(key_id, value_id);
                    if (!value_count) {
                        db_insert_key_value(key_id, value_id);
                    }
				}


                db_purge_old_data();
                db_clear_temp_tables();
                for (const auto& item : temp_container) {
                    const int64_t key_id = db_get_key_id(item.first);
                    if (key_id == -1) continue;
                    for (const auto& pair : item.second) {
                        const int64_t value_id = db_get_value_id(pair.first);
                        if (value_id == -1) continue;
                        db_set_value_count(key_id, value_id, pair.second);
                    }
                }
			} catch (...) {
                db_handle_reconcile_exception(std::current_exception());
            }
        }

        // Helper function to find or insert value in a sorted vector

        /// \brief Finds or inserts a value into a sorted vector.
        /// This method finds a value in the vector, or inserts it if it is not present. It uses
        /// a specialized comparison depending on the type of value.
        /// \tparam T The type of the value.
        /// \param vec The vector where the value will be searched or inserted.
        /// \param value The value to search or insert.
        /// \return Iterator to the position of the value in the vector.
        template<typename T>
        typename std::vector<std::pair<T, int>>::iterator find_or_insert(std::vector<std::pair<T, int>>& vec, const T& value,
            typename std::enable_if<
					!std::is_integral<T>::value &&
					!std::is_floating_point<T>::value &&
					!std::is_same<T, std::string>::value &&
					!std::is_same<T, std::vector<char>>::value &&
					!std::is_same<T, std::vector<uint8_t>>::value &&
					std::is_trivially_copyable<T>::value
				>::type* = 0) {
            auto it = std::find_if(vec.begin(), vec.end(), [&value](const std::pair<T, int>& element) {
                return byte_compare(element.first, value);
            });
            if (it == vec.end()) {
                //it = vec.insert(it, {value, 0});
                it = vec.emplace(it, std::make_pair(value, 0));
            }
            return it;
        }

        /// \brief Finds or inserts a value into a sorted vector.
        /// This method finds a value in the vector, or inserts it if it is not present. It uses
        /// a specialized comparison depending on the type of value.
        /// \tparam T The type of the value.
        /// \param vec The vector where the value will be searched or inserted.
        /// \param value The value to search or insert.
        /// \return Iterator to the position of the value in the vector.
        template<typename T>
        typename std::vector<std::pair<T, int>>::iterator find_or_insert(std::vector<std::pair<T, int>>& vec, const T& value,
            typename std::enable_if<
					std::is_integral<T>::value ||
					std::is_floating_point<T>::value ||
					std::is_same<T, std::string>::value ||
					std::is_same<T, std::vector<char>>::value ||
					std::is_same<T, std::vector<uint8_t>>::value>::type* = 0) {
            auto it = std::find_if(vec.begin(), vec.end(), [&value](const std::pair<T, int>& element) {
                return element.first == value;
            });
            if (it == vec.end()) {
                it = vec.insert(it, {value, 0});
            }
            return it;
        }

        /// \brief Reconciles the database with the provided container, ensuring data integrity.
        /// \tparam ContainerT Template for the container type.
        /// \param container Container with content to reconcile with the database.
        /// \param mode Transaction mode.
        /// \throws sqlite_exception if an SQLite error occurs.
        template<template <class...> class ContainerT, template <class...> class ValueContainerT>
		void db_reconcile(
                const ContainerT<KeyT, ValueContainerT<ValueT>>& container) {
            try {
                // Collect value repetitions
                std::unordered_map<KeyT, std::unordered_map<ValueT, int>> temp_container;
				for (const auto& pair : container) {
                    auto it_key = temp_container.find(pair.first);
                    for (const ValueT& value : pair.second) {
                        if (it_key == temp_container.end()) {
                            temp_container[pair.first][value] = 1;
                        } else {
                            auto it_value = it_key->second.find(value);
                            if (it_value == it_key->second.end()) {
                                it_key->second[value] = 1;
                            } else {
                                it_value->second++;
                            }
                        }
                    }
				}

				db_clear_temp_tables();
                for (const auto& item : temp_container) {
                    db_insert_key(item.first);
                    db_insert_key_temp(item.first);
                    for (const auto& pair : item.second) {
                        db_insert_value(pair.first);
                        db_insert_value_temp(pair.first);
                    }
                }

                for (const auto& pair : container) {
                    const int64_t key_id = db_get_key_id(pair.first);
                    if (key_id == -1) continue;
                    for (const ValueT& value : pair.second) {
                        const int64_t value_id = db_get_value_id(value);
                        if (value_id == -1) continue;
                        const size_t value_count = db_get_value_count(key_id, value_id);
                        if (value_count) continue;
                        db_insert_key_value(key_id, value_id);
                    }
				}

                db_purge_old_data();
                db_clear_temp_tables();
                for (const auto& item : temp_container) {
                    const int64_t key_id = db_get_key_id(item.first);
                    if (key_id == -1) continue;
                    for (const auto& pair : item.second) {
                        const int64_t value_id = db_get_value_id(pair.first);
                        if (value_id == -1) continue;
                        db_set_value_count(key_id, value_id, pair.second);
                    }
                }
			} catch (...) {
                db_handle_reconcile_exception(std::current_exception());
            }
        }

        /// \brief Handles an exception that occurs during reconciliation.
        /// \param ex The current exception that was caught (via std::current_exception()).
        /// \throws sqlite_exception after handling the exception.
        void db_handle_reconcile_exception(std::exception_ptr ex) {
            try {
                // Reset and clear bindings for statements
                std::vector<SqliteStmt*> stmts = {
                    &m_stmt_insert_key, &m_stmt_insert_value, &m_stmt_get_key_id,
                    &m_stmt_get_value_id, &m_stmt_get_value_count, &m_stmt_set_value_count,
                    &m_stmt_insert_key_temp, &m_stmt_insert_value_temp,
                    &m_stmt_purge_keys, &m_stmt_purge_values, &m_stmt_clear_keys_temp, &m_stmt_clear_values_temp
                };

                for (auto* stmt : stmts) {
                    stmt->reset();
                    stmt->clear_bindings();
                }
                if (ex) std::rethrow_exception(ex);
            } catch (const sqlite_exception &e) {
                // Rethrow the same sqlite_exception
                throw e;
            } catch (const std::exception &e) {
                // Wrap std::exception in sqlite_exception and throw
                throw sqlite_exception(e.what());
            } catch (...) {
                // Catch all other types of exceptions and throw a generic sqlite_exception
                throw sqlite_exception("Unknown error occurred while reconciling data.");
            }
		}

        /// \brief Finds values by key in the database.
        /// \tparam ContainerT Template for the container type.
        /// \param key The key to search for.
        /// \param values The container to store the values associated with the key.
        /// \return True if the key was found, false otherwise.
        /// \throws sqlite_exception if an SQLite error occurs.
        template<template <class...> class ContainerT>
        bool db_find(const KeyT &key, ContainerT<ValueT>& container) {
            int err;
            try {
                m_stmt_find.bind_value<KeyT>(1, key);
                while ((err = m_stmt_find.step()) == SQLITE_ROW) {
                    ValueT value = m_stmt_find.extract_column<ValueT>(0);
                    const size_t value_count = m_stmt_find.extract_column<size_t>(1);
                    add_value(container, value, value_count);
                }
                m_stmt_find.reset();
                return !container.empty();
            } catch (const sqlite_exception &e) {
                m_stmt_find.reset();
                throw e;
            } catch (const std::exception &e) {
                m_stmt_find.reset();
                throw sqlite_exception(e.what());
            } catch (...) {
                m_stmt_find.reset();
                throw sqlite_exception("Unknown error occurred while finding key-value pairs.");
            }
        }

        /// \brief Sets the value count for a key-value pair in the database.
        /// \param key The key.
        /// \param value The value.
        /// \param value_count The count of the value associated with the key.
        /// \throws sqlite_exception if an SQLite error occurs.
		void db_set_value_count_key_value(const KeyT& key, const ValueT& value, const size_t& value_count) {
            try {
                m_stmt_set_value_count_kv.bind_value<size_t>(1, value_count);
                m_stmt_set_value_count_kv.bind_value<KeyT>(2, key);
                m_stmt_set_value_count_kv.bind_value<ValueT>(3, value);
                m_stmt_set_value_count_kv.execute();
                m_stmt_set_value_count_kv.reset();
                m_stmt_set_value_count_kv.clear_bindings();
            } catch (const sqlite_exception &e) {
                m_stmt_set_value_count_kv.reset();
                m_stmt_set_value_count_kv.clear_bindings();
                throw e;
            } catch (const std::exception &e) {
                m_stmt_set_value_count_kv.reset();
                m_stmt_set_value_count_kv.clear_bindings();
                throw sqlite_exception(e.what());
            } catch (...) {
                m_stmt_set_value_count_kv.reset();
                m_stmt_set_value_count_kv.clear_bindings();
                throw sqlite_exception("Unknown error occurred while setting value count for key-value pair.");
            }
        }

        /// \brief Removes a specific key-value pair from the database.
        /// \param key The key of the pair to be removed.
        /// \param value The value of the pair to be removed.
        /// \throws sqlite_exception if an SQLite error occurs.
        void db_remove_key_value(const KeyT &key, const ValueT &value) {
            try {
                m_stmt_remove_key_value.bind_value<KeyT>(1, key);
                m_stmt_remove_key_value.bind_value<ValueT>(2, value);
                m_stmt_remove_key_value.execute();
                m_stmt_remove_key_value.reset();
                m_stmt_remove_key_value.clear_bindings();
            } catch (const sqlite_exception &e) {
                m_stmt_remove_key_value.reset();
                m_stmt_remove_key_value.clear_bindings();
                throw e;
            } catch (const std::exception &e) {
                m_stmt_remove_key_value.reset();
                m_stmt_remove_key_value.clear_bindings();
                throw sqlite_exception(e.what());
            } catch (...) {
                m_stmt_remove_key_value.reset();
                m_stmt_remove_key_value.clear_bindings();
                throw sqlite_exception("Unknown error occurred while removing key-value pair.");
            }
        }

        /// \brief Removes all values associated with a key from the database.
        /// \param key The key of the pairs to be removed.
        /// \throws sqlite_exception if an SQLite error occurs.
        void db_remove_all_values(const KeyT &key) {
            try {
                m_stmt_remove_all_values.bind_value<KeyT>(1, key);
                m_stmt_remove_all_values.execute();
                m_stmt_remove_all_values.reset();
                m_stmt_remove_all_values.clear_bindings();
            } catch (const sqlite_exception &e) {
                m_stmt_remove_all_values.reset();
                m_stmt_remove_all_values.clear_bindings();
                throw e;
            } catch (const std::exception &e) {
                m_stmt_remove_all_values.reset();
                m_stmt_remove_all_values.clear_bindings();
                throw sqlite_exception(e.what());
            } catch (...) {
                m_stmt_remove_all_values.reset();
                m_stmt_remove_all_values.clear_bindings();
                throw sqlite_exception("Unknown error occurred while removing values by key.");
            }
        }

        /// \brief Retrieves the count of a specific value associated with a key in the database.
        /// \param key The key.
        /// \param value The value.
        /// \return The count of the value associated with the key.
        /// \throws sqlite_exception if an SQLite error occurs.
        std::size_t db_get_value_count_key_value(const KeyT &key, const ValueT &value) const {
            int err;
            size_t value_count = 0;
            try {
                m_stmt_get_value_count_kv.bind_value<KeyT>(1, key);
                m_stmt_get_value_count_kv.bind_value<ValueT>(2, value);
                while ((err = m_stmt_get_value_count_kv.step()) == SQLITE_ROW) {
                    value_count = m_stmt_get_value_count_kv.extract_column<size_t>(0);
                }
                m_stmt_get_value_count_kv.reset();
                m_stmt_get_value_count_kv.clear_bindings();
                return value_count;
            } catch (const sqlite_exception &e) {
                m_stmt_get_value_count_kv.reset();
                m_stmt_get_value_count_kv.clear_bindings();
                throw e;
            } catch (const std::exception &e) {
                m_stmt_get_value_count_kv.reset();
                m_stmt_get_value_count_kv.clear_bindings();
                throw sqlite_exception(e.what());
            } catch (...) {
                m_stmt_get_value_count_kv.reset();
                m_stmt_get_value_count_kv.clear_bindings();
                throw sqlite_exception("Unknown error occurred while retrieving value count for key-value pair.");
            }
        }

        /// \brief Returns the number of elements in the database.
        /// \return The number of key-value pairs in the database.
        /// \throws sqlite_exception if an SQLite error occurs.
        std::size_t db_count_key() const {
            std::size_t count = 0;
            int err;
            try {
                for (;;) {
					while ((err = m_stmt_count_key.step()) == SQLITE_ROW) {
						count = m_stmt_count_key.extract_column<std::size_t>(0);
					}
					if (err == SQLITE_DONE) {
						m_stmt_count_key.reset();
						break;
					}
					if (err == SQLITE_BUSY) {
						// Handle busy database, retry reading
						m_stmt_count_key.reset();
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
            } catch(const sqlite_exception &e) {
                m_stmt_count_key.reset();
                throw e;
            } catch(const std::exception &e) {
                m_stmt_count_key.reset();
                throw sqlite_exception(e.what());
            } catch(...) {
                m_stmt_count_key.reset();
                throw sqlite_exception("Unknown error occurred.");
            }
            return count;
        }

		/// \brief Clears all key-value pairs from the database.
		/// \throws sqlite_exception if an SQLite error occurs.
		void db_clear() {
			try {
				m_stmt_clear_keys.execute();
				m_stmt_clear_keys.reset();
				m_stmt_clear_values.execute();
				m_stmt_clear_values.reset();
				m_stmt_clear_key_values.execute();
				m_stmt_clear_key_values.reset();
			} catch(const sqlite_exception &e) {
				db_handle_clear_exception(e);
			} catch(const std::exception &e) {
				db_handle_clear_exception(sqlite_exception(e.what()));
			} catch(...) {
				db_handle_clear_exception(sqlite_exception("Unknown error occurred."));
			}
		}

		void db_handle_clear_exception(const sqlite_exception &e) {
            m_stmt_clear_keys.reset();
            m_stmt_clear_values.reset();
            m_stmt_clear_key_values.reset();

            throw e;
		}

        /// \brief Inserts a key into the database.
        /// \param key The key to be inserted.
        /// \throws sqlite_exception if an SQLite error occurs.
        void db_insert_key(const KeyT& key) {
            m_stmt_insert_key.bind_value<KeyT>(1, key);
            m_stmt_insert_key.execute();
            m_stmt_insert_key.reset();
            m_stmt_insert_key.clear_bindings();
        }

        /// \brief Retrieves the ID of a key from the database.
        /// \param key The key whose ID is to be retrieved.
        /// \return The ID of the key, or -1 if the key is not found.
        /// \throws sqlite_exception if an SQLite error occurs.
        int64_t db_get_key_id(const KeyT& key) {
            m_stmt_get_key_id.bind_value<KeyT>(1, key);
            int64_t key_id = -1;
            if (m_stmt_get_key_id.step() == SQLITE_ROW) {
                key_id = m_stmt_get_key_id.extract_column<int64_t>(0);
            }
            m_stmt_get_key_id.reset();
            m_stmt_get_key_id.clear_bindings();
            return key_id;
        }

        /// \brief Inserts a value into the database.
        /// \param value The value to be inserted.
        /// \throws sqlite_exception if an SQLite error occurs.
        void db_insert_value(const ValueT& value) {
            m_stmt_insert_value.bind_value<ValueT>(1, value);
            m_stmt_insert_value.execute();
            m_stmt_insert_value.reset();
            m_stmt_insert_value.clear_bindings();
        }

        /// \brief Retrieves the ID of a value from the database.
        /// \param value The value whose ID is to be retrieved.
        /// \return The ID of the value, or -1 if the value is not found.
        /// \throws sqlite_exception if an SQLite error occurs.
        int64_t db_get_value_id(const ValueT& value) {
            m_stmt_get_value_id.bind_value<ValueT>(1, value);
            int64_t value_id = -1;
            if (m_stmt_get_value_id.step() == SQLITE_ROW) {
                value_id = m_stmt_get_value_id.extract_column<int64_t>(0);
            }
            m_stmt_get_value_id.reset();
            m_stmt_get_value_id.clear_bindings();
            return value_id;
        }

        /// \brief Inserts a key into the temporary keys table.
        /// \param key The key to be inserted into the temporary table.
        /// \throws sqlite_exception if an SQLite error occurs.
        void db_insert_key_temp(const KeyT& key) {
            m_stmt_insert_key_temp.bind_value<KeyT>(1, key);
            m_stmt_insert_key_temp.execute();
            m_stmt_insert_key_temp.reset();
            m_stmt_insert_key_temp.clear_bindings();
        }

        /// \brief Inserts a value into the temporary values table.
        /// \param value The value to be inserted into the temporary table.
        /// \throws sqlite_exception if an SQLite error occurs.
        void db_insert_value_temp(const ValueT& value) {
            m_stmt_insert_value_temp.bind_value<ValueT>(1, value);
            m_stmt_insert_value_temp.execute();
            m_stmt_insert_value_temp.reset();
            m_stmt_insert_value_temp.clear_bindings();
        }

        /// \brief Inserts a key-value pair into the key-value table.
        /// \param key_id The ID of the key.
        /// \param value_id The ID of the value.
        /// \throws sqlite_exception if an SQLite error occurs.
        void db_insert_key_value(const int64_t& key_id, const int64_t& value_id) {
            m_stmt_insert_key_value.bind_value<int64_t>(1, key_id);
            m_stmt_insert_key_value.bind_value<int64_t>(2, value_id);
            m_stmt_insert_key_value.execute();
            m_stmt_insert_key_value.reset();
            m_stmt_insert_key_value.clear_bindings();
        }

        /// \brief Retrieves the count of values associated with a specific key-value pair.
        /// \param key_id The ID of the key.
        /// \param value_id The ID of the value.
        /// \return The count of values associated with the key-value pair.
        /// \throws sqlite_exception if an SQLite error occurs.
        std::size_t db_get_value_count(const int64_t &key_id, const int64_t& value_id) {
            m_stmt_get_value_count.bind_value<int64_t>(1, key_id);
            m_stmt_get_value_count.bind_value<int64_t>(2, value_id);
            std::size_t value_count = 0;
            if (m_stmt_get_value_count.step() == SQLITE_ROW) {
                value_count = m_stmt_get_value_count.extract_column<std::size_t>(0);
            }
            m_stmt_get_value_count.reset();
            m_stmt_get_value_count.clear_bindings();
            return value_count;
        }

        /// \brief Sets the count of values associated with a specific key-value pair.
        /// \param key_id The ID of the key.
        /// \param value_id The ID of the value.
        /// \param value_count The count to be set.
        /// \throws sqlite_exception if an SQLite error occurs.
        void db_set_value_count(const int64_t& key_id, const int64_t& value_id, const size_t& value_count) {
            m_stmt_set_value_count.bind_value<size_t>(1, value_count);
            m_stmt_set_value_count.bind_value<int64_t>(2, key_id);
            m_stmt_set_value_count.bind_value<int64_t>(3, value_id);
            m_stmt_set_value_count.execute();
            m_stmt_set_value_count.reset();
            m_stmt_set_value_count.clear_bindings();
        }

        /// \brief Removes old keys and values that are no longer in the temporary tables.
        /// \throws sqlite_exception if an SQLite error occurs.
        void db_purge_old_data() {
			m_stmt_purge_keys.execute();
            m_stmt_purge_keys.reset();
            m_stmt_purge_values.execute();
            m_stmt_purge_values.reset();
		}

		/// \brief Clears the temporary keys and values tables.
        /// \throws sqlite_exception if an SQLite error occurs.
        void db_clear_temp_tables() {
			m_stmt_clear_keys_temp.execute();
            m_stmt_clear_keys_temp.reset();
            m_stmt_clear_values_temp.execute();
            m_stmt_clear_values_temp.reset();
		}

	}; // KeyMultiValueDB

}; // namespace sqlite_containers
