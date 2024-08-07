#include <sqlite_containers/KeyDB.hpp>
#include <iostream>
#include <set>

int main() {
    try {
        // Create database configuration
        sqlite_containers::Config config;
        config.db_path = "example-set.db";

        // Create KeyDB instance
        sqlite_containers::KeyDB<int> key_db(config);
        key_db.connect();

        // Clear the table for a fresh start
        key_db.clear();

        // Create a std::set with keys
        std::set<int> keys = {1, 2, 3, 4, 5};

        // Append the contents of the std::set to the database
        key_db.append(keys);

        // Retrieve all keys from the database and print them
        std::set<int> retrieved_keys_set = key_db.retrieve_all<std::set>();
        std::cout << "Keys in database after append: ";
        for (const auto& key : retrieved_keys_set) {
            std::cout << key << " ";
        }
        std::cout << std::endl;

        // Insert a new key
        key_db.insert(6);

        std::list<int> retrieved_keys_list = key_db.retrieve_all<std::list>();
        std::cout << "Keys in database after insert: ";
        for (const auto& key : retrieved_keys_list) {
            std::cout << key << " ";
        }
        std::cout << std::endl;

        // Check if the key exists in the database
        if (key_db.find(6)) {
            std::cout << "Key 6 found in the database." << std::endl;
        } else {
            std::cout << "Key 6 not found in the database." << std::endl;
        }

        // Check if a non-existing key is found
        if (key_db.find(10)) {
            std::cout << "Key 10 found in the database." << std::endl;
        } else {
            std::cout << "Key 10 not found in the database." << std::endl;
        }

        // Remove a key
        key_db.remove(3);

        // Retrieve all keys from the database after removal and print them
        std::vector<int> retrieved_keys_vector = key_db.retrieve_all<std::vector>();
        std::cout << "Keys in database after removing key 3: ";
        for (const auto& key : retrieved_keys_vector) {
            std::cout << key << " ";
        }
        std::cout << std::endl;

    } catch (const sqlite_containers::sqlite_exception &e) {
        std::cerr << "SQLite error: " << e.what() << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}
