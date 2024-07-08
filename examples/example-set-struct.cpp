#include <sqlite_containers/KeyDB.hpp>
#include <iostream>
#include <set>

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

    // Comparison operator for std::set and other sorted containers.
    bool operator<(const MyStruct& other) const {
        if (a != other.a) {
            return a < other.a;
        }
        return b < other.b;
    }
};

int main() {
    try {
        // Create database configuration
        sqlite_containers::Config config;
        config.db_path = "example-set-struct.db";

        // Create KeyDB instance
        sqlite_containers::KeyDB<MyStruct> key_db(config);
        key_db.connect();

        // Clear the table for a fresh start
        key_db.clear();

        // Create a std::set with keys
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
        std::cout << "Keys in database after append:" << std::endl;
        for (const auto& key : retrieved_keys_set) {
            std::cout << key << std::endl;
        }
        std::cout << std::endl;

        // Insert a new key
        key_db.insert({60, 1.0});
        std::list<MyStruct> retrieved_keys_list = key_db.retrieve_all<std::list>();
        std::cout << "Keys in database after insert:" << std::endl;
        for (const auto& key : retrieved_keys_list) {
            std::cout << key << std::endl;
        }
        std::cout << std::endl;

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
        std::cout << "Keys in database after removing key 3:" << std::endl;
        for (const auto& key : retrieved_keys_vector) {
            std::cout << key << std::endl;
        }
        std::cout << std::endl;

    } catch (const sqlite_containers::sqlite_exception &e) {
        std::cerr << "SQLite error: " << e.what() << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}
