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
		~KeyDB() override final = default;

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
		SqliteStmt m_stmt_load;         ///< Statement for loading data from the database.
		SqliteStmt m_stmt_replace;      ///< Statement for replacing key-value pairs in the database.
		SqliteStmt m_stmt_find;         ///< Statement for finding a key.
		SqliteStmt m_stmt_remove;	    ///< Statement for removing a key.
		SqliteStmt m_stmt_clear;	    ///< Statement for clearing the table.

		/// \brief Creates the table in the database.
		/// \param config Configuration settings.
		void db_create_table(const Config &config) override final {
			const std::string table_name = config.table_name.empty() ? "key_store" : config.table_name;

			// Create table if they do not exist
			const std::string create_table_sql =
				"CREATE TABLE IF NOT EXISTS " + table_name + " ("
				"key " + get_sqlite_type<KeyT>() + " PRIMARY KEY NOT NULL);";
			execute(m_sqlite_db, create_table_sql);

			// Initialize prepared statements
			m_stmt_load.init(m_sqlite_db, "SELECT key FROM " + table_name + ";");
			m_stmt_replace.init(m_sqlite_db, "REPLACE INTO " + table_name + " (key) VALUES (?);");
			m_stmt_find.init(m_sqlite_db, "SELECT EXISTS(SELECT 1 FROM " + table_name + " WHERE key = ?);");
			m_stmt_remove.init(m_sqlite_db, "DELETE FROM " + table_name + " WHERE key = ?;");
			m_stmt_clear.init(m_sqlite_db, "DELETE FROM " + table_name);
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
						return is_found;
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
			} catch(const sqlite_exception &e) {
				m_stmt_find.reset();
				m_stmt_find.clear_bindings();
				throw e;
			} catch(const std::exception &e) {
				m_stmt_find.reset();
				m_stmt_find.clear_bindings();
				throw sqlite_exception(e.what());
			} catch(...) {
				m_stmt_find.reset();
				m_stmt_find.clear_bindings();
				throw sqlite_exception("Unknown error occurred.");
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

		/// \brief Inserts a key into the database.
		/// \param key The key to be inserted.
		/// \throws sqlite_exception if an SQLite error occurs.
		void db_insert(const KeyT &key) {
			try {
				m_stmt_replace.bind_value<KeyT>(1, key);
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
