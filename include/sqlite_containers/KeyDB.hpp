#pragma once

/// \file KeyDB.hpp
/// \brief Declaration of the KeyDB class for managing keys in a SQLite database.

#include "parts/BaseDB.hpp"

namespace sqlite_containers {

	/// \brief Template class for managing keys in a SQLite database.
	/// \tparam KeyT Type of the keys.
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
		~KeyDB() override = default;

		/// \brief Synchronizes the database content to a set.
		/// \tparam ContainerT Template for the container type.
		/// \param set Container to be synchronized with database content.
		/// \throws sqlite_exception if an SQLite error occurs.
		template<template <class...> class ContainerT>
		void sync_to_set(ContainerT<KeyT>& set) {
			std::lock_guard<std::mutex> locker(m_sqlite_mutex);
			db_sync_to_set(set);
		}

		/// \brief Synchronizes the database content to a set with a transaction.
		/// \tparam ContainerT Template for the container type.
		/// \param set Container to be synchronized with database content.
		/// \param mode Transaction mode.
		/// \throws sqlite_exception if an SQLite error occurs.
		template<template <class...> class ContainerT>
		void sync_to_set(
				ContainerT<KeyT>& set,
				const TransactionMode& mode) {
			std::lock_guard<std::mutex> locker(m_sqlite_mutex);
			try {
				db_begin(mode);
				db_sync_to_set(set);
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
		/// \tparam ContainerT Template for the container type.
		/// \return A container with all keys.
		/// \throws sqlite_exception if an SQLite error occurs.
		template<template <class...> class ContainerT>
		ContainerT<KeyT> retrieve_all() {
			ContainerT<KeyT> set;
			std::unique_lock<std::mutex> locker(m_sqlite_mutex);
			db_sync_to_set(set);
			locker.unlock();
			return set;
		}

		/// \brief Retrieves all keys from the database with a transaction.
		/// \tparam ContainerT Template for the container type.
		/// \param mode Transaction mode.
		/// \return A container with all keys.
		/// \throws sqlite_exception if an SQLite error occurs.
		template<template <class...> class ContainerT>
		ContainerT<KeyT> retrieve_all(const TransactionMode& mode) {
			ContainerT<KeyT> set;
			std::unique_lock<std::mutex> locker(m_sqlite_mutex);
			db_sync_to_set(set);
			locker.unlock();
			return set;
		}

		/// \brief Synchronizes the set content to the database.
		/// \tparam ContainerT Template for the container type.
		/// \param set Container with content to be synchronized to the database.
		/// \throws sqlite_exception if an SQLite error occurs.
		template<template <class...> class ContainerT>
		void sync_to_db(const ContainerT<KeyT>& set) {
			std::lock_guard<std::mutex> locker(m_sqlite_mutex);
			db_sync_to_db(set);
		}

		/// \brief Synchronizes the set content to the database with a transaction.
		/// \tparam ContainerT Template for the container type.
		/// \param set Container with content to be synchronized to the database.
		/// \param mode Transaction mode.
		/// \throws sqlite_exception if an SQLite error occurs.
		template<template <class...> class ContainerT>
		void sync_to_db(const ContainerT<KeyT>& set, const TransactionMode& mode) {
			std::lock_guard<std::mutex> locker(m_sqlite_mutex);
			try {
				db_begin(mode);
				sync_to_db(set);
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
		SqliteStmt m_stmt_sync_to_set;  ///< Statement for synchronizing to set.
		SqliteStmt m_stmt_sync_to_db;   ///< Statement for synchronizing to database.
		SqliteStmt m_stmt_find;         ///< Statement for finding a key.
		SqliteStmt m_stmt_remove;	    ///< Statement for removing a key.
		SqliteStmt m_stmt_clear;	    ///< Statement for clearing the table.

		/// \brief Creates the table in the database.
		/// \param config Configuration settings.
		void db_create_table(const Config &config) override final {
			const std::string table_name = config.table_name.empty() ? "key_store" : config.table_name;
			const std::string create_key_table_sql =
				"CREATE TABLE IF NOT EXISTS " + table_name + " ("
				"key " + get_sqlite_type<KeyT>() + " PRIMARY KEY NOT NULL);";
			execute(m_sqlite_db, create_key_table_sql);
			m_stmt_sync_to_set.init(m_sqlite_db, "SELECT key FROM " + table_name + ";");
			m_stmt_sync_to_db.init(m_sqlite_db, "REPLACE INTO " + table_name + " (key) VALUES (?);");
			m_stmt_find.init(m_sqlite_db, "SELECT EXISTS(SELECT 1 FROM " + table_name + " WHERE key = ?);");
			m_stmt_remove.init(m_sqlite_db, "DELETE FROM " + table_name + " WHERE key == ?;");
			m_stmt_clear.init(m_sqlite_db, "DELETE FROM " + table_name);
		}

		/// \brief Synchronizes the database content to a set.
		/// \tparam ContainerT Template for the container type.
		/// \param set Container to be synchronized with database content.
		/// \throws sqlite_exception if an SQLite error occurs.
		template<template <class...> class ContainerT>
		void db_sync_to_set(ContainerT<KeyT>& set) {
			int err;
			try {
				for (;;) {
					while ((err = m_stmt_sync_to_set.step()) == SQLITE_ROW) {
						KeyT key = m_stmt_sync_to_set.extract_column<KeyT>(0);
						set.insert(key);
					}
					if (err == SQLITE_DONE) {
						m_stmt_sync_to_set.reset();
						return;
					}
					if (err == SQLITE_BUSY) {
						// Restart reading
						m_stmt_sync_to_set.reset();
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
				m_stmt_sync_to_set.reset();
				throw e;
			} catch(const std::exception &e) {
				m_stmt_sync_to_set.reset();
				throw sqlite_exception(e.what());
			} catch(...) {
				m_stmt_sync_to_set.reset();
				throw sqlite_exception("Unknown error occurred.");
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
						return is_found;
					}
					if (err == SQLITE_BUSY) {
						// Restart reading
						m_stmt_find.reset();
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
				m_stmt_find.reset();
				throw e;
			} catch(const std::exception &e) {
				m_stmt_find.reset();
				throw sqlite_exception(e.what());
			} catch(...) {
				m_stmt_find.reset();
				throw sqlite_exception("Unknown error occurred.");
			}
		}

		/// \brief Synchronizes the set content to the database.
		/// \tparam ContainerT Template for the container type.
		/// \param set Container with content to be synchronized to the database.
		/// \throws sqlite_exception if an SQLite error occurs.
		template<template <class...> class ContainerT>
		void db_sync_to_db(const ContainerT<KeyT>& set) {
			try {
				for (const auto& item : set) {
					m_stmt_sync_to_db.bind_value<KeyT>(1, item);
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

		/// \brief Inserts a key into the database.
		/// \param key The key to be inserted.
		/// \throws sqlite_exception if an SQLite error occurs.
		void db_insert(const KeyT &key) {
			try {
				m_stmt_sync_to_db.bind_value<KeyT>(1, key);
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

		/// \brief Removes a key from the database.
		/// \param key The key to be removed.
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

		/// \brief Clears all keys from the database.
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

	}; // KeyDB

}; // namespace sqlite_containers
