#include <sqlite_containers/KeyValueDB.hpp>
#include <iostream>
#include <map>
#include <vector>
#include <sstream>

int main() {
    // Database configuration creation
    sqlite_containers::Config config;
    config.db_path = "example_vector.db";

    // Creation of KeyValueDB instance
    sqlite_containers::KeyValueDB<int, std::vector<char>> map_db;
    map_db.connect(config);

    // Inserting several key-value pairs
    map_db.insert(1, {'a', 'b', 'c'});
    map_db.insert(2, {'d', 'e', 'f'});
    map_db.insert(3, {'g', 'h', 'i'});

    // Finding and printing a value
    std::vector<char> value;
    if (map_db.find(2, value)) {
        std::cout << "Found value for key 2: ";
        for (char c : value) {
            std::cout << c;
        }
        std::cout << std::endl;
    } else {
        std::cout << "Key 2 not found." << std::endl;
    }

    // Synchronizing the database with std::map
    std::map<int, std::vector<char>> my_map;
    map_db.sync_to_map(my_map);

    // Printing all key-value pairs
    std::cout << "Contents of my_map after sync_to_map:" << std::endl;
    for (const auto& pair : my_map) {
        std::cout << "Key: " << pair.first << ", Value: ";
        for (char c : pair.second) {
            std::cout << c;
        }
        std::cout << std::endl;
    }

    // Retrieving all key-value pairs directly from the database
    std::map<int, std::vector<char>> all_entries = map_db.retrieve_all<std::map>();
    std::cout << "Contents of the database using retrieve_all:" << std::endl;
    for (const auto& pair : all_entries) {
        std::cout << "Key: " << pair.first << ", Value: ";
        for (char c : pair.second) {
            std::cout << c;
        }
        std::cout << std::endl;
    }

    // Removing a key-value pair
    map_db.remove(3);
    all_entries = map_db.retrieve_all<std::map>();
    std::cout << "Contents of the database after removing key 3:" << std::endl;
    for (const auto& pair : all_entries) {
        std::cout << "Key: " << pair.first << ", Value: ";
        for (char c : pair.second) {
            std::cout << c;
        }
        std::cout << std::endl;
    }

    // Inserting a new key-value pair
    map_db.insert(4, {'j', 'k', 'l'});
    all_entries = map_db.retrieve_all<std::map>();
    std::cout << "Contents of the database after inserting key 4:" << std::endl;
    for (const auto& pair : all_entries) {
        std::cout << "Key: " << pair.first << ", Value: ";
        for (char c : pair.second) {
            std::cout << c;
        }
        std::cout << std::endl;
    }

    // Synchronizing std::map with the database
    my_map[5] = {'m', 'n', 'o'};
    map_db.sync_to_db(my_map);

    all_entries = map_db.retrieve_all<std::map>();
    std::cout << "Contents of the database after sync_to_db:" << std::endl;
    for (const auto& pair : all_entries) {
        std::cout << "Key: " << pair.first << ", Value: ";
        for (char c : pair.second) {
            std::cout << c;
        }
        std::cout << std::endl;
    }

    // Clearing the database
    map_db.clear();
    all_entries = map_db.retrieve_all<std::map>();
    std::cout << "Contents of the database after clear:" << std::endl;
    if (all_entries.empty()) {
        std::cout << "Database is empty." << std::endl;
    } else {
        for (const auto& pair : all_entries) {
            std::cout << "Key: " << pair.first << ", Value: ";
            for (char c : pair.second) {
                std::cout << c;
            }
            std::cout << std::endl;
        }
    }

    return 0;
}
