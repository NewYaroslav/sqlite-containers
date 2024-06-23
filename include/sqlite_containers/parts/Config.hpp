#pragma once

/// \file Config.hpp
/// \brief Contains the declaration of Config class for SQLite database configuration.

#include "Enums.hpp"

namespace sqlite_containers {

    /// \brief Configuration class for SQLite database settings.
    class Config {
    public:
        std::string db_path;                    ///< Path to the SQLite database file.
        std::string table_name;                 ///< Name of the database table.
        bool read_only = false;                 ///< Whether the database is in read-only mode.
        bool use_uri = false;                   ///< Whether to use URI for opening the database.
        bool in_memory = false;                 ///< Whether the database should be in-memory.
        bool use_async = false;                 ///< Whether to use asynchronous write.
        int user_version = -1;                  ///< User-defined version number for the database schema.
        int busy_timeout = 1000;                ///< Timeout in milliseconds for busy handler.
        int page_size = 4096;                   ///< SQLite page size.
        int cache_size = 2000;                  ///< SQLite cache size (in pages).
        int analysis_limit = 1000;              ///< Maximum number of rows to analyze.
        int wal_autocheckpoint = 1000;          ///< WAL auto-checkpoint threshold.
        JournalMode journal_mode = JournalMode::DELETE;          ///< SQLite journal mode.
        SynchronousMode synchronous = SynchronousMode::FULL;     ///< SQLite synchronous mode.
        LockingMode locking_mode = LockingMode::NORMAL;          ///< SQLite locking mode.
        AutoVacuumMode auto_vacuum_mode = AutoVacuumMode::NONE;  ///< SQLite auto-vacuum mode.

        /// \brief Default constructor.
        Config() = default;
    };

}; // namespace sqlite_containers
