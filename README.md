# SQLite Containers

SQLite Containers is a lightweight header-only C++ library designed to provide seamless integration between SQLite databases and standard C++ containers, such as std::map and other associative containers. This library abstracts the complexity of database operations, allowing developers to interact with SQLite databases using familiar container interfaces. It ensures data persistence while providing robust mechanisms for synchronization between in-memory data structures and persistent storage.

## Features

- **Container Integration:** Supports synchronization of SQLite databases with various standard C++ containers, including std::map and other associative containers.
- **Thread-Safety:** Built-in mechanisms to ensure thread-safe interactions with the database.
- **Transaction Management:** Provides transaction support to ensure data integrity during complex operations.
- **Customizable Configuration:** Allows configuration of database paths, table names, and other settings.
- **Exception Handling:** Comprehensive exception handling to manage SQLite errors gracefully.
- **Easy-to-Use API:** Simplified API for common database operations like insert, remove, find, sync, and clear.

## Classes

### KeyDB
The `KeyDB` class is designed for storing unique keys and allows working with containers like `std::set`, `std::unordered_set`, `std::list`, and `std::vector`.

### KeyValueDB
The `KeyValueDB` class stores key-value pairs and allows working with `std::map` and `std::unordered_map`.

### KeyMultiValueDB
The `KeyMultiValueDB` class implements storage of key-value pairs, where the database follows a many-to-many relationship model. `KeyMultiValueDB` allows working with `std::map`, `std::unordered_map`, `std::multimap`, and `std::unordered_multimap`, and also supports using other containers as values of the map, such as `std::map<int, std::set<std::string>>`.

## Installation

To use the SQLite Containers library, simply include the source files in your project and ensure that you have the SQLite3 library installed.

Ensure that SQLite is compiled with the *SQLITE_THREADSAFE=1* option to provide multithreading support.

## Usage Example

Here’s a basic example demonstrating how to use `KeyMultiValueDB` with various containers:

```cpp
#include <sqlite_containers/KeyMultiValueDB.hpp>
#include <iostream>

struct MyStruct {
    int64_t a;
    double b;

    // Serialization and deserialization of the struct
    friend std::ostream& operator<<(std::ostream& os, const MyStruct& ms) {
        os << ms.a << " " << ms.b;
        return os;
    }

    friend std::istream& operator>>(std::istream& is, MyStruct& ms) {
        is >> ms.a >> ms.b;
        return is;
    }
};

int main() {
    try {
        // Create database configuration
        sqlite_containers::Config config;
        config.db_path = "example-multimap-struct.db";

        // Create KeyMultiValueDB instance
        sqlite_containers::KeyMultiValueDB<int, MyStruct> key_value_db(config);
        key_value_db.connect();

        // Clear the table for a fresh start
        key_value_db.clear();

        // Create a std::multimap with key-value pairs
        std::multimap<int, MyStruct> multimap_pairs = {
            {1, {10, 1.1}},
            {2, {20, 2.2}},
            {2, {20, 2.2}},
            {2, {20, 2.2}},
            {1, {14, 4.0}},
            {3, {15, 4.0}},
            {2, {30, 1.3}}
        };

        // Create a std::map with std::set as values
        std::map<int, std::list<MyStruct>> map_with_set_pairs = {
            {3, {{15, 4.0}}},
            {1, {{10, 1.1}}},
        };

        // Append the contents of the std::multimap to the database
        key_value_db.append(multimap_pairs);

        // Append the contents of the std::map with std::set values to the database
        key_value_db.append(map_with_set_pairs);

        // Retrieve all key-value pairs from the database and print them
        auto retrieved_key_value_pairs = key_value_db.retrieve_all<std::multimap>();
        std::cout << "Key-value pairs in database after append:" << std::endl;
        for (const auto& pair : retrieved_key_value_pairs) {
            std::cout << pair.first << " -> " << pair.second << std::endl;
        }

        // Insert a new key-value pair
        key_value_db.insert(4, {50, 7.7});
        key_value_db.insert(4, {50, 7.7});

        // Retrieve all key-value pairs from the database into a std::map with std::list values and print them
        using map_with_list_t = std::map<int, std::list<MyStruct>>;
        map_with_list_t retrieved_map_with_list_pairs = key_value_db.retrieve_all<std::map, std::list>();
        std::cout << "Key-value pairs in database after insert:" << std::endl;
        for (const auto& pair : retrieved_map_with_list_pairs) {
            for (const auto& item : pair.second) {
                std::cout << pair.first << " -> " << item << std::endl;
            }
        }

        // Check if the key exists in the database
        std::list<MyStruct> values; // or std::vector<MyStruct>, std::set<MyStruct>
        if (key_value_db.find(4, values)) {
            std::cout << "Key 4 found in the database with values:" << std::endl;
            for (const auto& value : values) {
                std::cout << value << std::endl;
            }
        } else {
            std::cout << "Key 4 not found in the database." << std::endl;
        }

        // Check if a non-existing key is found
        values.clear();
        if (key_value_db.find(10, values)) {
            std::cout << "Key 10 found in the database with values:" << std::endl;
            for (const auto& value : values) {
                std::cout << value << std::endl;
            }
        } else {
            std::cout << "Key 10 not found in the database." << std::endl;
        }

        // Remove a specific key-value pair
        key_value_db.remove(2, {20, 2.2});

        // Remove all values associated with a key
        key_value_db.remove(1);

        // Retrieve all key-value pairs from the database after removal and print them
        retrieved_key_value_pairs = key_value_db.retrieve_all<std::multimap>();
        std::cout << "Key-value pairs in database after removals:" << std::endl;
        for (const auto& pair : retrieved_key_value_pairs) {
            std::cout << pair.first << " -> " << pair.second << std::endl;
        }

        // Reconcile the database with the contents of the std::multimap
        key_value_db.reconcile(multimap_pairs);

        // Retrieve all key-value pairs from the database after reconcile and print them
        retrieved_key_value_pairs = key_value_db.retrieve_all<std::multimap>();
        std::cout << "Key-value pairs in database after reconcile:" << std::endl;
        for (const auto& pair : retrieved_key_value_pairs) {
            std::cout << pair.first << " -> " << pair.second << std::endl;
        }

    } catch (const sqlite_containers::sqlite_exception &e) {
        std::cerr << "SQLite error: " << e.what() << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
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