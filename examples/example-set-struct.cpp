#include <sqlite_containers/KeyDB.hpp>
#include <iostream>
#include <set>
#include <vector>
#include <list>

// Structure MyStruct with support for serialization and deserialization
struct MyStruct {
    int64_t a;
    double b;

    // Serialization of MyStruct to an output stream
    friend std::ostream& operator<<(std::ostream& os, const MyStruct& ms) {
        os << ms.a << " " << ms.b;
        return os;
    }

    // Deserialization of MyStruct from an input stream
    friend std::istream& operator>>(std::istream& is, MyStruct& ms) {
        is >> ms.a >> ms.b;
        return is;
    }

    // Comparison operator for std::set and other sorted containers
    bool operator<(const MyStruct& other) const {
        if (a != other.a) {
            return a < other.a;
        }
        return b < other.b;
    }
};

// Utility function to print contents of a set
template <typename SetType>
void print_set(const SetType& set, const std::string& header) {
    std::cout << header << std::endl;
    for (const auto& key : set) {
        std::cout << key << std::endl;
    }
    std::cout << std::endl;
}

// Utility function to print contents of a list or vector
template <typename ListType>
void print_list(const ListType& list, const std::string& header) {
    std::cout << header << std::endl;
    for (const auto& key : list) {
        std::cout << key << std::endl;
    }
    std::cout << std::endl;
}

int main() {
    try {
        // Create the database configuration
        sqlite_containers::Config config;
        config.db_path = "example-set-struct.db";

        // Create KeyDB instance for working with MyStruct keys
        sqlite_containers::KeyDB<MyStruct> key_db(config);
        key_db.connect();

        // Clear the table for a fresh start
        key_db.clear();

        // Create a std::set of keys
        std::set<MyStruct> keys = {
            {10, 1.0},
            {20, 3.0},
            {30, 4.0},
            {40, 5.0},
            {50, 6.0}
        };

        // Append the contents of the std::set to the database
        key_db.append(keys);

        // Retrieve all keys from the database and print them
        std::set<MyStruct> retrieved_keys_set = key_db.retrieve_all<std::set>();
        print_set(retrieved_keys_set, "Keys in database after append:");

        // Insert a new key
        key_db.insert({60, 1.0});
        std::list<MyStruct> retrieved_keys_list = key_db.retrieve_all<std::list>();
        print_list(retrieved_keys_list, "Keys in database after insert:");

        // Check if the key exists in the database
        if (key_db.find({60, 1.0})) {
            std::cout << "Key {60, 1.0} found in the database." << std::endl;
        } else {
            std::cout << "Key {60, 1.0} not found in the database." << std::endl;
        }

        // Check if a non-existing key is found
        if (key_db.find({100, 8.0})) {
            std::cout << "Key {100, 8.0} found in the database." << std::endl;
        } else {
            std::cout << "Key {100, 8.0} not found in the database." << std::endl;
        }

        // Remove a key
        key_db.remove({30, 4.0});

        // Retrieve all keys from the database after removal and print them
        std::vector<MyStruct> retrieved_keys_vector = key_db.retrieve_all<std::vector>();
        print_list(retrieved_keys_vector, "Keys in database after removing key {30, 4.0}:");

        // Print the number of keys and check if the database is empty
        std::cout << "Number of keys in the database: " << key_db.count() << std::endl;
        std::cout << "Is the database empty? " << (key_db.empty() ? "Yes" : "No") << std::endl;

    } catch (const sqlite_containers::sqlite_exception &e) {
        std::cerr << "SQLite error: " << e.what() << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}
