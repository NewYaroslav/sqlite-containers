#pragma once

/// \file Utils.hpp
/// \brief Utility functions for working with SQLite in sqlite_containers.

#include <sqlite3.h>
#include <stdexcept>
#include <string>
#include <thread>
#include <chrono>
#include <vector>
#include <string>
#include <cstring>
#include <type_traits>

#define SQLITE_CONTAINERS_BUSY_RETRY_DELAY_MS 50

namespace sqlite_containers {

	/// \brief Exception class for SQLite errors.
	class sqlite_exception : public std::runtime_error {
	public:
		/// \brief Constructs a new sqlite_exception with the given message.
		/// param message The error message.
		/// param errorCode The SQLite error code.
		explicit sqlite_exception(const std::string &message, const int &error_code = -1) :
			std::runtime_error(message), m_error_code(error_code) {}

		/// brief Returns the SQLite error code associated with this exception.
		/// return The SQLite error code.
		int error_code() const noexcept {
			return m_error_code;
		}
	private:
		int m_error_code; // The SQLite error code
	}; // sqlite_exception

	/// \brief Executes a SQLite statement.
	/// \param stmt Pointer to the SQLite statement.
	/// \throws sqlite_exception if statement is null, or if an error occurs during execution.
	inline void execute(sqlite3_stmt *stmt) {
		if (!stmt) throw sqlite_exception("Invalid statement pointer.");
		int err;
		for (;;) {
			while ((err = sqlite3_step(stmt)) == SQLITE_ROW);
			switch (err) {
			case SQLITE_DONE:
				return;
			case SQLITE_BUSY:
				sqlite3_sleep(SQLITE_CONTAINERS_BUSY_RETRY_DELAY_MS);
				continue;
			case SQLITE_FULL:
				throw sqlite_exception("Disk full or IO error.", err);
			case SQLITE_IOERR:
				throw sqlite_exception("Failed to insert data into database.", err);
			default:
				throw sqlite_exception("SQLite error.", err);
			}
		}
	}

	/// \brief Executes a SQLite statement.
	/// \param sqlite_db Pointer to the SQLite database.
	/// \param stmt Pointer to the SQLite statement.
	/// \throws sqlite_exception if the database or statement is null, or if an error occurs during execution.
	inline void execute(sqlite3 *sqlite_db, sqlite3_stmt *stmt) {
		if (!sqlite_db || !stmt) throw sqlite_exception("Invalid database or statement pointer.");
		int err;
		for (;;) {
			while ((err = sqlite3_step(stmt)) == SQLITE_ROW);
			switch (err) {
			case SQLITE_DONE:
				return;
			case SQLITE_BUSY:
				sqlite3_sleep(SQLITE_CONTAINERS_BUSY_RETRY_DELAY_MS);
				continue;
			case SQLITE_FULL:
				throw sqlite_exception("Disk full or IO error: " + std::string(sqlite3_errmsg(sqlite_db)) + ". Error code: " + std::to_string(err), err);
			case SQLITE_IOERR:
				throw sqlite_exception("Failed to insert data into database: " + std::string(sqlite3_errmsg(sqlite_db)) + ". Error code: " + std::to_string(err), err);
			default:
				throw sqlite_exception("SQLite error: " + std::string(sqlite3_errmsg(sqlite_db)) + ". Error code: " + std::to_string(err), err);
			}
		}
	}

	/// \brief Prepares and executes a SQLite statement given as a string.
	/// \param sqlite_db Pointer to the SQLite database.
	/// \param query The SQL query to execute.
	/// \throws sqlite_exception if the database pointer is null, the request is empty, or if an error occurs during execution.
	inline void execute(sqlite3 *sqlite_db, const char *query) {
		if (!sqlite_db) throw sqlite_exception("Invalid database pointer.");
		if (!query || std::strlen(query) == 0) throw sqlite_exception("Empty SQL request.");
		int err;
		do {
			err = sqlite3_exec(sqlite_db, query, nullptr, nullptr, nullptr);
			if (err == SQLITE_BUSY) {
				sqlite3_sleep(SQLITE_CONTAINERS_BUSY_RETRY_DELAY_MS);
			} else
			if (err != SQLITE_OK) {
				std::string err_msg = "SQLite error during prepare: ";
				err_msg += sqlite3_errmsg(sqlite_db);
				err_msg += ". Error code: ";
				err_msg += std::to_string(err);
				throw sqlite_exception(err_msg, err);
			}
		} while (err == SQLITE_BUSY);
	}

	/// \brief Prepares and executes a SQLite statement given as a string.
	/// \param sqlite_db Pointer to the SQLite database.
	/// \param query The SQL query to execute.
	/// \throws sqlite_exception if the database pointer is null, the request is empty, or if an error occurs during execution.
	inline void execute(sqlite3 *sqlite_db, const std::string &query) {
		execute(sqlite_db, query.c_str());
	}

//------------------------------------------------------------------------------

	/// \brief Gets the SQLite data type as a string for the given type.
	/// \return The SQLite data type as a string.
	template<typename T>
	inline std::string get_sqlite_type(
			typename std::enable_if<std::is_integral<T>::value>::type* = 0) {
		return "INTEGER";
	}

	template<typename T>
	inline std::string get_sqlite_type(
			typename std::enable_if<std::is_floating_point<T>::value>::type* = 0) {
		return "REAL";
	}

	template<typename T>
	inline std::string get_sqlite_type(
			typename std::enable_if<std::is_same<T, std::string>::value>::type* = 0) {
		return "TEXT";
	}

	template<typename T>
	inline std::string get_sqlite_type(
			typename std::enable_if<
				!std::is_integral<T>::value &&
				!std::is_floating_point<T>::value &&
				std::is_trivially_copyable<T>::value>::type* = 0) {
		return "BLOB";
	}

	template<typename T>
	inline typename std::enable_if<
		std::is_trivially_copyable<
			typename T::value_type>::value &&
			std::is_same<T, std::vector<typename T::value_type>>::value,
			std::string
		>::type get_sqlite_type() {
		return "BLOB";
	}

}; // namespace sqlite_containers
