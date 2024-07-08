#pragma once

/// \file KeyValueDB.hpp
/// \brief Declaration of the KeyValueDB class for managing key-value pairs in a SQLite database.

#include "parts/BaseDB.hpp"

namespace sqlite_containers {

	/// \brief Template class for managing key-value pairs in a SQLite database.
	/// \tparam KeyT Type of the keys.
	/// \tparam ValueT Type of the values.
	template<class KeyT, class ValueT>
	class KeyValueDB final : public BaseDB {
	public:

		/// \brief Default constructor.
		KeyValueDB() : BaseDB() {}

		/// \brief Constructor with configuration.
		/// \param config Configuration settings for the database.
		KeyValueDB(const Config& config) : BaseDB() {
			set_config(config);
		}

		/// \brief Destructor.
		~KeyValueDB() override final = default;

		/// \brief Loads data from the database into the container.
		/// \tparam ContainerT Template for the container type (map or unordered_map).
		/// \param container Container to be synchronized with database content.
		/// \throws sqlite_exception if an SQLite error occurs.
		template<template <class...> class ContainerT>
		void load(ContainerT<KeyT, ValueT>& container) {
			std::lock_guard<std::mutex> locker(m_sqlite_mutex);
			db_load(container);
		}

		/// \brief Loads data from the database into the container with a transaction.
		/// \tparam ContainerT Template for the container type (map or unordered_map).
		/// \param container Container to be synchronized with database content.
		/// \param mode Transaction mode.
		/// \throws sqlite_exception if an SQLite error occurs.
		template<template <class...> class ContainerT>
		void load(
				ContainerT<KeyT, ValueT>& container,
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

		/// \brief Retrieves all key-value pairs from the database.
		/// \tparam ContainerT Template for the container type (map or unordered_map)..
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
		/// \tparam ContainerT Template for the container type (map or unordered_map)..
		/// \param mode Transaction mode.
		/// \return A container with all key-value pairs.
		/// \throws sqlite_exception if an SQLite error occurs.
		template<template <class...> class ContainerT>
		ContainerT<KeyT, ValueT> retrieve_all(const TransactionMode& mode) {
			ContainerT<KeyT, ValueT> container;
			std::unique_lock<std::mutex> locker(m_sqlite_mutex);
			try {
				db_begin(mode);
				db_load(container);
				db_commit();
                locker.unlock();
                return container;
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

		/// \brief Appends the content of the container to the database.
		/// \tparam ContainerT Template for the container type (map or unordered_map)..
		/// \param container Container with content to be synchronized to the database.
		/// \throws sqlite_exception if an SQLite error occurs.
		template<template <class...> class ContainerT>
		void append(const ContainerT<KeyT, ValueT>& container) {
			std::lock_guard<std::mutex> locker(m_sqlite_mutex);
			db_append(container);
		}

		/// \brief Appends the content of the container to the database with a transaction.
		/// \tparam ContainerT Template for the container type (map or unordered_map)..
		/// \param container Container with content to be synchronized to the database.
		/// \param mode Transaction mode.
		/// \throws sqlite_exception if an SQLite error occurs.
		template<template <class...> class ContainerT>
		void append(const ContainerT<KeyT, ValueT>& container, const TransactionMode& mode) {
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

		/// \brief Finds a value by key in the database.
		/// \param key The key to search for.
		/// \param value The value associated with the key.
		/// \return True if the key was found, false otherwise.
		/// \throws sqlite_exception if an SQLite error occurs.
		bool find(const KeyT &key, ValueT &value) {
			std::lock_guard<std::mutex> locker(m_sqlite_mutex);
			return db_find(key, value);
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
		SqliteStmt m_stmt_remove;	    ///< Statement for removing key-value pair from the database.
		SqliteStmt m_stmt_clear;	    ///< Statement for clearing the table in the database.

		/// \brief Creates the table in the database.
		/// \param config Configuration settings.
		void db_create_table(const Config &config) override final {
			const std::string table_name = config.table_name.empty() ? "kv_store" : config.table_name;

			// Create table if they do not exist
			const std::string create_table_sql =
				"CREATE TABLE IF NOT EXISTS " + table_name + " ("
				"key " + get_sqlite_type<KeyT>() + " PRIMARY KEY NOT NULL,"
				"value " + get_sqlite_type<ValueT>() + "		 NOT NULL);";
			execute(m_sqlite_db, create_table_sql);

            // Initialize prepared statements
			m_stmt_load.init(m_sqlite_db, "SELECT key, value FROM " + table_name + ";");
			m_stmt_replace.init(m_sqlite_db, "REPLACE INTO  " + table_name + " (key, value) VALUES (?, ?);");
			m_stmt_get_value.init(m_sqlite_db, "SELECT value FROM " + table_name + " WHERE key = ?;");
			m_stmt_remove.init(m_sqlite_db, "DELETE FROM " + table_name + " WHERE key = ?;");
			m_stmt_clear.init(m_sqlite_db, "DELETE FROM " + table_name);
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
			} catch(const sqlite_exception &e) {
				m_stmt_load.reset();
				throw e;
			} catch(const std::exception &e) {
				m_stmt_load.reset();
				throw sqlite_exception(e.what());
			} catch(...) {
				m_stmt_load.reset();
				throw sqlite_exception("Unknown error occurred.");
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
			} catch(const sqlite_exception &e) {
				m_stmt_get_value.reset();
				m_stmt_get_value.clear_bindings();
				throw e;
			} catch(const std::exception &e) {
				m_stmt_get_value.reset();
				m_stmt_get_value.clear_bindings();
				throw sqlite_exception(e.what());
			} catch(...) {
				m_stmt_get_value.reset();
				m_stmt_get_value.clear_bindings();
				throw sqlite_exception("Unknown error occurred.");
			}
			return false;
		}

		/// \brief Appends the content of the container to the database.
		/// \tparam ContainerT Template for the container type (map or unordered_map).
		/// \param container Container with content to be synchronized to the database.
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
			} catch(const sqlite_exception &e) {
				m_stmt_replace.reset();
				m_stmt_replace.clear_bindings();
				throw e;
			} catch(const std::exception &e) {
				m_stmt_replace.reset();
				m_stmt_replace.clear_bindings();
				throw sqlite_exception(e.what());
			} catch(...) {
				m_stmt_replace.reset();
				m_stmt_replace.clear_bindings();
				throw sqlite_exception("Unknown error occurred.");
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
			} catch(const sqlite_exception &e) {
				m_stmt_replace.reset();
				m_stmt_replace.clear_bindings();
				throw e;
			} catch(const std::exception &e) {
				m_stmt_replace.reset();
				m_stmt_replace.clear_bindings();
				throw sqlite_exception(e.what());
			} catch(...) {
				m_stmt_replace.reset();
				m_stmt_replace.clear_bindings();
				throw sqlite_exception("Unknown error occurred.");
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
			} catch(const sqlite_exception &e) {
				m_stmt_remove.reset();
				m_stmt_remove.clear_bindings();
				throw e;
			} catch(const std::exception &e) {
				m_stmt_remove.reset();
				m_stmt_remove.clear_bindings();
				throw sqlite_exception(e.what());
			} catch(...) {
				m_stmt_remove.reset();
				m_stmt_remove.clear_bindings();
				throw sqlite_exception("Unknown error occurred.");
			}
		}

		/// \brief Clears all key-value pairs from the database.
		/// \throws sqlite_exception if an SQLite error occurs.
		void db_clear() {
			try {
				m_stmt_clear.execute();
				m_stmt_clear.reset();
			} catch(const sqlite_exception &e) {
				m_stmt_clear.reset();
				throw e;
			} catch(const std::exception &e) {
				m_stmt_clear.reset();
				throw sqlite_exception(e.what());
			} catch(...) {
				m_stmt_clear.reset();
				throw sqlite_exception("Unknown error occurred.");
			}
		}

	}; // KeyValueDB

}; // namespace sqlite_containers
