#pragma once

/// \file SqliteStmt.hpp
/// \brief Declaration of the SqliteStmt class for managing SQLite prepared statements.

#include "Utils.hpp"

namespace sqlite_containers {

	/// \brief Class for managing SQLite prepared statements.
	class SqliteStmt {
	public:

		/// \brief Default constructor.
		SqliteStmt() = default;

		/// \brief Constructs a SqliteStmt and prepares the statement.
		/// \param sqlite_db Pointer to the SQLite database.
		/// \param query SQL query to prepare.
		/// \throws sqlite_exception if the query preparation fails.
		SqliteStmt(sqlite3 *sqlite_db, const char *query) {
			init(sqlite_db, query);
		}

		/// \brief Constructs a SqliteStmt and prepares the statement.
		/// \param sqlite_db Pointer to the SQLite database.
		/// \param query SQL query to prepare.
		/// \throws sqlite_exception if the query preparation fails.
		SqliteStmt(sqlite3 *sqlite_db, const std::string &query) {
			init(sqlite_db, query);
		}

		/// \brief Destructor.
		~SqliteStmt() {
			if (!m_stmt) return;
			sqlite3_finalize(m_stmt);
			m_stmt = nullptr;
		}

		/// \brief Initializes the statement.
		/// \param sqlite_db Pointer to the SQLite database.
		/// \param query SQL query to prepare.
		/// \throws sqlite_exception if the query preparation fails.
		void init(sqlite3 *sqlite_db, const char *query) {
			int err;
			do {
				err = sqlite3_prepare_v2(sqlite_db, query, -1, &m_stmt, nullptr);
				if (err == SQLITE_BUSY) {
					sqlite3_sleep(SQLITE_CONTAINERS_BUSY_RETRY_DELAY_MS);
				} else
				if (err != SQLITE_OK) {
					std::string err_msg = "Failed to prepare SQL statement: ";
					err_msg += std::string(query);
					err_msg += ". Error code: ";
					err_msg += std::to_string(err);
					throw sqlite_exception(err_msg, err);
				}
			} while (err == SQLITE_BUSY);
		}

		/// \brief Initializes the statement.
		/// \param sqlite_db Pointer to the SQLite database.
		/// \param query SQL query to prepare.
		/// \throws sqlite_exception if the query preparation fails.
		void init(sqlite3 *sqlite_db, const std::string &query) {
			init(sqlite_db, query.c_str());
		}

		/// \brief Gets the prepared SQLite statement.
		/// \return Pointer to the prepared SQLite statement.
		sqlite3_stmt *get_stmt() noexcept {
			return m_stmt;
		}

		/// \brief Resets the prepared statement.
		/// \throws sqlite_exception if the reset operation fails.
		void reset() {
			const int err = sqlite3_reset(m_stmt);
			if (err == SQLITE_OK) return;
			throw sqlite_exception("Failed to reset SQL statement. Error code: " + std::to_string(err), err);
		}

		/// \brief Clears all bindings on the prepared statement.
		/// \throws sqlite_exception if the clear bindings operation fails.
		void clear_bindings() {
			const int err = sqlite3_clear_bindings(m_stmt);
			if (err == SQLITE_OK) return;
			throw sqlite_exception("Failed to clear bindings on SQL statement. Error code: " + std::to_string(err), err);
		}

		/// \brief Executes the prepared statement.
		/// \param sqlite_db Pointer to the SQLite database.
		/// \throws sqlite_exception if the execution fails.
		void execute(sqlite3 *sqlite_db) {
			sqlite_containers::execute(sqlite_db, m_stmt);
		}

		/// \brief Executes the prepared statement.
		/// \throws sqlite_exception if the execution fails.
		void execute() {
			sqlite_containers::execute(m_stmt);
		}

		/// brief Advances the prepared statement to the next result row or completion.
		/// return Result code: SQLITE_ROW for a new row, SQLITE_DONE for completion, or an error code.
		int step() {
			return sqlite3_step(m_stmt);
		}

		/// \brief Extracts a value from a SQLite statement column.
		/// \param index Index of the column to extract.
		/// \return The extracted value.
		template<typename T>
		inline T extract_column(const int &index,
				typename std::enable_if<std::is_integral<T>::value>::type* = 0) {
			return static_cast<T>(sqlite3_column_int64(m_stmt, index));
		}

		template<typename T>
		inline T extract_column(const int &index,
				typename std::enable_if<std::is_floating_point<T>::value>::type* = 0) {
			return static_cast<T>(sqlite3_column_double(m_stmt, index));
		}

		template<typename T>
		inline T extract_column(const int &index,
				typename std::enable_if<std::is_same<T, std::string>::value>::type* = 0) {
			const unsigned char *text = sqlite3_column_text(m_stmt, index);
			if (text) {
				return std::string(reinterpret_cast<const char*>(text));
			}
			return std::string();
		}

		template<typename T>
		inline T extract_column(const int &index,
				typename std::enable_if<std::is_same<T, std::vector<char>>::value>::type* = 0) {
			const void* blob = sqlite3_column_blob(m_stmt, index);
			const int blob_size = sqlite3_column_bytes(m_stmt, index);
			return std::vector<char>(static_cast<const char*>(blob), static_cast<const char*>(blob) + blob_size);
		}

		template<typename T>
		inline T extract_column(const int &index,
				typename std::enable_if<std::is_same<T, std::vector<uint8_t>>::value>::type* = 0) {
			const void* blob = sqlite3_column_blob(m_stmt, index);
			const int blob_size = sqlite3_column_bytes(m_stmt, index);
			return std::vector<uint8_t>(static_cast<const uint8_t*>(blob), static_cast<const uint8_t*>(blob) + blob_size);
		}

		template<typename T>
		inline T extract_column(const int &index,
				typename std::enable_if<
					!std::is_integral<T>::value &&
					!std::is_floating_point<T>::value &&
					!std::is_same<T, std::string>::value &&
					!std::is_same<T, std::vector<char>>::value &&
					!std::is_same<T, std::vector<uint8_t>>::value &&
					std::is_trivially_copyable<T>::value
				>::type* = 0) {
			T value;
			const void* blob = sqlite3_column_blob(m_stmt, index);
			const int blob_size = sqlite3_column_bytes(m_stmt, index);
			if (blob_size == sizeof(T)) {
				std::memcpy(&value, blob, sizeof(T));
			} else {
				throw sqlite_exception("Blob size does not match POD size.");
			}
			return value;
		}

		/// \brief Binds a value to a SQLite statement.
		/// \param index Index of the parameter to bind.
		/// \param value The value to bind.
		/// \return True if the value was successfully bound, otherwise false.
		template<typename T>
		inline bool bind_value(const int &index, const T& value,
				typename std::enable_if<std::is_integral<T>::value>::type* = 0) {
			return (sqlite3_bind_int64(m_stmt, index, static_cast<int64_t>(value)) == SQLITE_OK);
		}

		template<typename T>
		inline bool bind_value(const int &index, const T& value,
				typename std::enable_if<std::is_floating_point<T>::value>::type* = 0) {
			return (sqlite3_bind_double(m_stmt, index, static_cast<double>(value)) == SQLITE_OK);
		}

		template<typename T>
		inline bool bind_value(const int &index, const T& value,
				typename std::enable_if<std::is_same<T, std::string>::value>::type* = 0) {
			return (sqlite3_bind_text(m_stmt, index, value.c_str(), -1, SQLITE_STATIC) == SQLITE_OK);
		}

		template<typename T>
		inline bool bind_value(const int &index, const T& value,
				typename std::enable_if<std::is_same<T, std::vector<char>>::value>::type* = 0) {
			return (sqlite3_bind_blob(m_stmt, index, value.data(), value.size(), SQLITE_STATIC) == SQLITE_OK);
		}

		template<typename T>
		inline bool bind_value(const int &index, const T& value,
				typename std::enable_if<std::is_same<T, std::vector<uint8_t>>::value>::type* = 0) {
			return (sqlite3_bind_blob(m_stmt, index, value.data(), value.size(), SQLITE_STATIC) == SQLITE_OK);
		}

		template<typename T>
		inline bool bind_value(const int &index, const T& value,
				typename std::enable_if<
					!std::is_integral<T>::value &&
					!std::is_floating_point<T>::value &&
					!std::is_same<T, std::string>::value &&
					!std::is_same<T, std::vector<char>>::value &&
					!std::is_same<T, std::vector<uint8_t>>::value &&
					std::is_trivially_copyable<T>::value
				>::type* = 0) {
			return (sqlite3_bind_blob(m_stmt, index, &value, sizeof(T), SQLITE_STATIC) == SQLITE_OK);
		}

	private:
		sqlite3_stmt *m_stmt = nullptr; ///< Pointer to the prepared SQLite statement.
	}; // SqliteStmt

}; // namespace sqlite_containers
