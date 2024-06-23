#pragma once

/// \file MapDB.hpp
/// \brief Declaration of the MapDB class for managing key-value pairs in a SQLite database.

#include "parts/BaseDB.hpp"

namespace sqlite_containers {

	/// \brief Template class for managing key-value pairs in a SQLite database.
	/// \tparam KeyT Type of the keys.
	/// \tparam ValueT Type of the values.
	template<class KeyT, class ValueT>
	class MapDB final : public BaseDB {
	public:

		/// \brief Default constructor.
		MapDB() : BaseDB() {}

		/// \brief Constructor with configuration.
		/// \param config Configuration settings for the database.
		MapDB(const Config& config) : BaseDB() {
			set_config(config);
		}

		/// \brief Destructor.
		~MapDB() override = default;

		/// \brief Synchronizes the database content to a map.
		/// \tparam ContainerT Template for the container type.
		/// \param map Container to be synchronized with database content.
		/// \throws sqlite_exception if an SQLite error occurs.
		template<template <class...> class ContainerT>
		void sync_to_map(ContainerT<KeyT, ValueT>& map) {
			std::lock_guard<std::mutex> locker(m_sqlite_mutex);
			db_sync_to_map(map);
		}

		/// \brief Synchronizes the database content to a map with a transaction.
		/// \tparam ContainerT Template for the container type.
		/// \param map Container to be synchronized with database content.
		/// \param mode Transaction mode.
		/// \throws sqlite_exception if an SQLite error occurs.
		template<template <class...> class ContainerT>
		void sync_to_map(
				ContainerT<KeyT, ValueT>& map,
				const TransactionMode& mode) {
			std::lock_guard<std::mutex> locker(m_sqlite_mutex);
			try {
				db_begin(mode);
				db_sync_to_map(map);
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
		/// \tparam ContainerT Template for the container type.
		/// \return A container with all key-value pairs.
		/// \throws sqlite_exception if an SQLite error occurs.
		template<template <class...> class ContainerT>
		ContainerT<KeyT, ValueT> retrieve_all() {
			ContainerT<KeyT, ValueT> map;
			std::unique_lock<std::mutex> locker(m_sqlite_mutex);
			db_sync_to_map(map);
			locker.unlock();
			return map;
		}

		/// \brief Retrieves all key-value pairs from the database with a transaction.
		/// \tparam ContainerT Template for the container type.
		/// \param mode Transaction mode.
		/// \return A container with all key-value pairs.
		/// \throws sqlite_exception if an SQLite error occurs.
		template<template <class...> class ContainerT>
		ContainerT<KeyT, ValueT> retrieve_all(const TransactionMode& mode) {
			ContainerT<KeyT, ValueT> map;
			std::unique_lock<std::mutex> locker(m_sqlite_mutex);
			db_sync_to_map(map);
			locker.unlock();
			return map;
		}

		/// \brief Synchronizes the map content to the database.
		/// \tparam ContainerT Template for the container type.
		/// \param map Container with content to be synchronized to the database.
		/// \throws sqlite_exception if an SQLite error occurs.
		template<template <class...> class ContainerT>
		void sync_to_db(const ContainerT<KeyT, ValueT>& map) {
			std::lock_guard<std::mutex> locker(m_sqlite_mutex);
			db_sync_to_db(map);
		}

		/// \brief Synchronizes the map content to the database with a transaction.
		/// \tparam ContainerT Template for the container type.
		/// \param map Container with content to be synchronized to the database.
		/// \param mode Transaction mode.
		/// \throws sqlite_exception if an SQLite error occurs.
		template<template <class...> class ContainerT>
		void sync_to_db(const ContainerT<KeyT, ValueT>& map, const TransactionMode& mode) {
			std::lock_guard<std::mutex> locker(m_sqlite_mutex);
			try {
				db_begin(mode);
				sync_to_db(map);
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
		SqliteStmt m_stmt_sync_to_map; ///< Statement for synchronizing to map.
		SqliteStmt m_stmt_sync_to_db;  ///< Statement for synchronizing to database.
		SqliteStmt m_stmt_get_value;   ///< Statement for getting value by key.
		SqliteStmt m_stmt_remove;	   ///< Statement for removing key-value pair.
		SqliteStmt m_stmt_clear;	   ///< Statement for clearing the table.

		/// \brief Creates the table in the database.
		/// \param config Configuration settings.
		void db_create_table(const Config &config) override final {
			const std::string table_name = config.table_name.empty() ? "kv_store" : config.table_name;
			const std::string create_key_table_sql =
				"CREATE TABLE IF NOT EXISTS " + table_name + " ("
				"key " + get_sqlite_type<KeyT>() + " PRIMARY KEY NOT NULL,"
				"value " + get_sqlite_type<ValueT>() + "		 NOT NULL);";
			execute(m_sqlite_db, create_key_table_sql);
			m_stmt_sync_to_map.init(m_sqlite_db, "SELECT key, value FROM " + table_name + ";");
			m_stmt_sync_to_db.init(m_sqlite_db, "REPLACE INTO  " + table_name + " (key, value) VALUES (?, ?);");
			m_stmt_get_value.init(m_sqlite_db, "SELECT value FROM " + table_name + " WHERE key == ?;");
			m_stmt_remove.init(m_sqlite_db, "DELETE FROM " + table_name + " WHERE key == ?;");
			m_stmt_clear.init(m_sqlite_db, "DELETE FROM " + table_name);
		}

		/// \brief Synchronizes the database content to a map.
		/// \tparam ContainerT Template for the container type.
		/// \param map Container to be synchronized with database content.
		/// \throws sqlite_exception if an SQLite error occurs.
		template<template <class...> class ContainerT>
		void db_sync_to_map(ContainerT<KeyT, ValueT>& map) {
			int err;
			try {
				for (;;) {
					while ((err = m_stmt_sync_to_map.step()) == SQLITE_ROW) {
						KeyT key = m_stmt_sync_to_map.extract_column<KeyT>(0);
						ValueT value = m_stmt_sync_to_map.extract_column<ValueT>(1);
						map[key] = value;
					}
					if (err == SQLITE_DONE) {
						m_stmt_sync_to_map.reset();
						return;
					}
					if (err == SQLITE_BUSY) {
						// Restart reading
						m_stmt_sync_to_map.reset();
						sqlite3_sleep(SQLITE_CONTAINERS_BUSY_RETRY_DELAY_MS);
						continue;
					}
					std::string err_msg = "SQLite error: ";
					err_msg += std::to_string(err);
					err_msg += ", ";
					err_msg += sqlite3_errmsg(m_sqlite_db);
					throw sqlite_exception(err_msg, err);
				}
			} catch(const sqlite_exception &e) {
				m_stmt_sync_to_map.reset();
				throw e;
			} catch(const std::exception &e) {
				m_stmt_sync_to_map.reset();
				throw sqlite_exception(e.what());
			} catch(...) {
				m_stmt_sync_to_map.reset();
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
						return is_found;
					}
					if (err == SQLITE_BUSY) {
						// Restart reading
						m_stmt_get_value.reset();
						sqlite3_sleep(SQLITE_CONTAINERS_BUSY_RETRY_DELAY_MS);
						continue;
					}
					std::string err_msg = "SQLite error: ";
					err_msg += std::to_string(err);
					err_msg += ", ";
					err_msg += sqlite3_errmsg(m_sqlite_db);
					throw sqlite_exception(err_msg, err);
				}
			} catch(const sqlite_exception &e) {
				m_stmt_get_value.reset();
				throw e;
			} catch(const std::exception &e) {
				m_stmt_get_value.reset();
				throw sqlite_exception(e.what());
			} catch(...) {
				m_stmt_get_value.reset();
				throw sqlite_exception("Unknown error occurred.");
			}
			return false;
		}

		/// \brief Synchronizes the map content to the database.
		/// \tparam ContainerT Template for the container type.
		/// \param map Container with content to be synchronized to the database.
		/// \throws sqlite_exception if an SQLite error occurs.
		template<template <class...> class ContainerT>
		void db_sync_to_db(const ContainerT<KeyT, ValueT>& map) {
			try {
				for (const auto& pair : map) {
					m_stmt_sync_to_db.bind_value<KeyT>(1, pair.first);
					m_stmt_sync_to_db.bind_value<ValueT>(2, pair.second);
					m_stmt_sync_to_db.execute();
					m_stmt_sync_to_db.reset();
					m_stmt_sync_to_db.clear_bindings();
				}
			} catch(const sqlite_exception &e) {
				m_stmt_sync_to_db.reset();
				throw e;
			} catch(const std::exception &e) {
				m_stmt_sync_to_db.reset();
				throw sqlite_exception(e.what());
			} catch(...) {
				m_stmt_sync_to_db.reset();
				throw sqlite_exception("Unknown error occurred.");
			}
		}

		/// \brief Inserts a key-value pair into the database.
		/// \param key The key to be inserted.
		/// \param value The value to be inserted.
		/// \throws sqlite_exception if an SQLite error occurs.
		void db_insert(const KeyT &key, const ValueT &value) {
			try {
				m_stmt_sync_to_db.bind_value<KeyT>(1, key);
				m_stmt_sync_to_db.bind_value<ValueT>(2, value);
				m_stmt_sync_to_db.execute();
				m_stmt_sync_to_db.reset();
				m_stmt_sync_to_db.clear_bindings();
			} catch(const sqlite_exception &e) {
				m_stmt_sync_to_db.reset();
				throw e;
			} catch(const std::exception &e) {
				m_stmt_sync_to_db.reset();
				throw sqlite_exception(e.what());
			} catch(...) {
				m_stmt_sync_to_db.reset();
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
				throw e;
			} catch(const std::exception &e) {
				m_stmt_remove.reset();
				throw sqlite_exception(e.what());
			} catch(...) {
				m_stmt_remove.reset();
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

	}; // MapDB

}; // namespace sqlite_containers
