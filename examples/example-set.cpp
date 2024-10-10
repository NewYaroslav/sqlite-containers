#include <sqlite_containers/KeyDB.hpp>
#include <iostream>
#include <set>
#include <list>
#include <vector>

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

        // Check the number of keys in the database
        std::cout << "Number of keys in the database: " << key_db.count() << std::endl;

        // Check if the database is empty
        std::cout << "Is the database empty? " << (key_db.empty() ? "Yes" : "No") << std::endl;

        // Insert a new key
        key_db.insert(6);

        // Retrieve all keys and print them as a std::list
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

        // Example of using operator= to synchronize with the contents of a new std::set
        std::set<int> new_keys = {10, 20, 30};
        key_db = new_keys;  // Using operator= to synchronize
        std::vector<int> keys_after_assignment = key_db.retrieve_all<std::vector>();
        std::cout << "Keys in database after using operator= with new set: ";
        for (const auto& key : keys_after_assignment) {
            std::cout << key << " ";
        }
        std::cout << std::endl;

        // Example of using operator() to load keys from the database into a std::list
        std::list<int> keys_loaded_with_operator = key_db.template operator()<std::list>();
        std::cout << "Keys loaded using operator(): ";
        for (const auto& key : keys_loaded_with_operator) {
            std::cout << key << " ";
        }
        std::cout << std::endl;

        // Check the number of keys in the database after operations
        std::cout << "Number of keys in the database: " << key_db.count() << std::endl;

        // Check if the database is empty after operations
        std::cout << "Is the database empty? " << (key_db.empty() ? "Yes" : "No") << std::endl;

    } catch (const sqlite_containers::sqlite_exception &e) {
        std::cerr << "SQLite error: " << e.what() << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}
