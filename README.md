# SQLite Containers

SQLite Containers is a lightweight header-only C++ library designed to provide seamless integration between SQLite databases and standard C++ containers, such as std::map and other associative containers. This library abstracts the complexity of database operations, allowing developers to interact with SQLite databases using familiar container interfaces. It ensures data persistence while providing robust mechanisms for synchronization between in-memory data structures and persistent storage.

## Features

- **Container Integration:** Supports synchronization of SQLite databases with various standard C++ containers, including std::map and other associative containers.
- **Thread-Safety:** Built-in mechanisms to ensure thread-safe interactions with the database.
- **Transaction Management:** Provides transaction support to ensure data integrity during complex operations.
- **Customizable Configuration:** Allows configuration of database paths, table names, and other settings.
- **Exception Handling:** Comprehensive exception handling to manage SQLite errors gracefully.
- **Easy-to-Use API:** Simplified API for common database operations like insert, remove, find, sync, and clear.

## Installation

To use the SQLite Containers library, simply include the source files in your project and ensure that you have the SQLite3 library installed.

Ensure that SQLite is compiled with the *SQLITE_THREADSAFE=1* option to provide multithreading support.

## Usage Example

Here’s a basic example demonstrating how to use KeyValueDB with std::map:

```cpp
#include "sqlite_containers/KeyValueDB.hpp"
#include <iostream>
#include <map>

int main() {
    // Create a configuration for the database
    sqlite_containers::Config config;
    config.db_path = "example.db";

    // Create a KeyValueDB instance
    sqlite_containers::KeyValueDB<int, std::string> map_db(config);
    map_db.connect();

    // Insert some key-value pairs
    map_db.insert(1, "value1");
    map_db.insert(2, "value2");
    map_db.insert(3, "value3");

    // Find and print a value
    std::string value;
    if (map_db.find(2, value)) {
        std::cout << "Found value for key 2: " << value << std::endl;
    } else {
        std::cout << "Key 2 not found." << std::endl;
    }

    // Synchronize database to std::map
    std::map<int, std::string> my_map;
    map_db.sync_to_map(my_map);

    // Print all key-value pairs
    std::cout << "Contents of my_map after sync_to_map:" << std::endl;
    for (const auto& pair : my_map) {
        std::cout << "Key: " << pair.first << ", Value: " << pair.second << std::endl;
    }

    // Retrieve all key-value pairs directly from the database
    std::map<int, std::string> all_entries = map_db.retrieve_all<std::map>();
    std::cout << "Contents of database using retrieve_all:" << std::endl;
    for (const auto& pair : all_entries) {
        std::cout << "Key: " << pair.first << ", Value: " << pair.second << std::endl;
    }

    // Remove a key-value pair
    map_db.remove(3);
    all_entries = map_db.retrieve_all<std::map>();
    std::cout << "Contents of database after removing key 3:" << std::endl;
    for (const auto& pair : all_entries) {
        std::cout << "Key: " << pair.first << ", Value: " << pair.second << std::endl;
    }

    // Insert a new key-value pair
    map_db.insert(4, "value4");
    all_entries = map_db.retrieve_all<std::map>();
    std::cout << "Contents of database after inserting key 4:" << std::endl;
    for (const auto& pair : all_entries) {
        std::cout << "Key: " << pair.first << ", Value: " << pair.second << std::endl;
    }

    // Synchronize std::map to database
    my_map[5] = "value5";
    map_db.sync_to_db(my_map);

    all_entries = map_db.retrieve_all<std::map>();
    std::cout << "Contents of database after sync_to_db:" << std::endl;
    for (const auto& pair : all_entries) {
        std::cout << "Key: " << pair.first << ", Value: " << pair.second << std::endl;
    }

    // Clear the database
    map_db.clear();
    all_entries = map_db.retrieve_all<std::map>();
    std::cout << "Contents of database after clear:" << std::endl;
    if (all_entries.empty()) {
        std::cout << "Database is empty." << std::endl;
    } else {
        for (const auto& pair : all_entries) {
            std::cout << "Key: " << pair.first << ", Value: " << pair.second << std::endl;
        }
    }

    return 0;
}

```

## Documentation

The documentation is under development.

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgements

Special thanks to the SQLite development team for their work on the SQLite library.