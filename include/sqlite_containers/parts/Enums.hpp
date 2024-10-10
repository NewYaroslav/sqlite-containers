#pragma once

/// \file Enums.hpp
/// \brief Contains enumerations and utility functions related to SQLite modes.

#include <array>
#include <string>

namespace sqlite_containers {

	/// \enum JournalMode
	/// \brief SQLite journal modes enumeration.
	enum class JournalMode {
		DELETE_MODE,///< Delete journal mode.
		TRUNCATE,	///< Truncate journal mode.
		PERSIST,	///< Persist journal mode.
		MEMORY,		///< Memory journal mode.
		WAL,		///< Write-ahead logging (WAL) mode.
		OFF			///< Off journal mode.
	};

	/// \enum SynchronousMode
	/// \brief SQLite synchronous modes enumeration.
	enum class SynchronousMode {
		OFF,		///< Synchronous mode off.
		NORMAL,		///< Normal synchronous mode.
		FULL,		///< Full synchronous mode.
		EXTRA		///< Extra synchronous mode.
	};

	/// \enum LockingMode
	/// \brief SQLite locking modes enumeration.
	enum class LockingMode {
		NORMAL,		///< Normal locking mode.
		EXCLUSIVE	///< Exclusive locking mode.
	};

	/// \enum AutoVacuumMode
	/// \brief SQLite auto-vacuum modes enumeration.
	enum class AutoVacuumMode {
		NONE,		///< No auto-vacuuming.
		FULL,		///< Full auto-vacuuming.
		INCREMENTAL ///< Incremental auto-vacuuming.
	};

	/// \enum TempStore
	/// \brief SQLite temporary storage modes enumeration.
	enum class TempStore {
		DEFAULT,	///< Default temporary storage behavior.
		FILE,		///< Temporary storage using a file.
		MEMORY		///< Temporary storage using memory.
	};

	/// \enum TransactionMode
	/// \brief Defines SQLite transaction modes.
	enum class TransactionMode {
		DEFERRED,	///< Waits to lock the database until a write operation is requested.
		IMMEDIATE,	///< Locks the database for writing at the start, allowing only read operations by others.
		EXCLUSIVE	///< Locks the database for both reading and writing, blocking other transactions.
	};

	/// \brief Converts JournalMode enum to string representation.
	/// \param mode The JournalMode enum value.
	/// \return String representation of the JournalMode.
	std::string to_string(const JournalMode &mode) {
		static const std::array<std::string, 6> data = {
			"DELETE",
			"TRUNCATE",
			"PERSIST",
			"MEMORY",
			"WAL",
			"OFF"
		};
		return data[static_cast<size_t>(mode)];
	}

	/// \brief Converts SynchronousMode enum to string representation.
	/// \param mode The SynchronousMode enum value.
	/// \return String representation of the SynchronousMode.
	std::string to_string(const SynchronousMode &mode) {
		static const std::array<std::string, 6> data = {
			"OFF",
			"NORMAL",
			"FULL",
			"EXTRA"
		};
		return data[static_cast<size_t>(mode)];
	}

	/// \brief Converts LockingMode enum to string representation.
	/// \param mode The LockingMode enum value.
	/// \return String representation of the LockingMode.
	std::string to_string(const LockingMode &mode) {
		static const std::array<std::string, 6> data = {
			"NORMAL",
			"EXCLUSIVE"
		};
		return data[static_cast<size_t>(mode)];
	}

	/// \brief Converts AutoVacuumMode enum to string representation.
	/// \param mode The AutoVacuumMode enum value.
	/// \return String representation of the AutoVacuumMode.
	std::string to_string(const AutoVacuumMode &mode) {
		static const std::array<std::string, 6> data = {
			"NONE",
			"FULL",
			"INCREMENTAL"
		};
		return data[static_cast<size_t>(mode)];
	}

	/// \brief Converts TransactionMode enum to string representation.
	/// \param mode The TransactionMode enum value.
	/// \return String representation of the TransactionMode.
	std::string to_string(const TransactionMode &mode) {
		static const std::array<std::string, 3> data = {
			"DEFERRED",
			"IMMEDIATE",
			"EXCLUSIVE"
		};
		return data[static_cast<size_t>(mode)];
	}

}; // namespace sqlite_containers
