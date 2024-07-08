#include <sqlite_containers/KeyValueDB.hpp>
#include <iostream>
#include <map>

int main() {
    // Create a configuration for the database
    sqlite_containers::Config config;
    config.db_path = "example-map.db";

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

    // Load the database contents into a std::map
    std::map<int, std::string> my_map;
    map_db.load(my_map);

    // Print all key-value pairs
    std::cout << "Contents of my_map after load:" << std::endl;
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

    // Append the contents of the std::map to the database
    my_map[5] = "value5";
    map_db.append(my_map);

    all_entries = map_db.retrieve_all<std::map>();
    std::cout << "Contents of database after append:" << std::endl;
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
