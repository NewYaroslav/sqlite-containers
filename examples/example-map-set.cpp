#include <iostream>
#include <sqlite_containers/KeyMultiValueDB.hpp>
#include <map>
#include <set>

// Function to print the contents of a map with std::set as values
template <typename MapType>
void print_map_with_set(const MapType& map, const std::string& header) {
    std::cout << header << std::endl;
    for (const auto& pair : map) {
        if (!pair.second.empty()) {
            for (const auto& item : pair.second) {
                std::cout << "Key: " << pair.first << " -> Value: " << item << std::endl;
            }
        } else {
            std::cout << "Key: " << pair.first << " has an empty set." << std::endl;
        }
    }
}

int main() {
    try {
        // Create database configuration
        sqlite_containers::Config config;
        config.db_path = "example-map-set.db";  // Path to the database

        // Create KeyMultiValueDB instance for int keys and std::set<int> values
        sqlite_containers::KeyMultiValueDB<int, int> key_value_db(config);
        key_value_db.connect();  // Connect to the database

        // Clear the table for a fresh start
        key_value_db.clear();

        // Create a std::map with std::set as values
        std::map<int, std::set<int>> map_with_set_pairs = {
            {3, {1, 2}},
            {1, {}},   // Empty set for key 1
        };

        // Append the contents of the std::map with std::set values to the database
        std::cout << "Appending data to the database using reconcile..." << std::endl;
        key_value_db.reconcile(map_with_set_pairs);  // Synchronize the map with the database

        // Retrieve all key-value pairs from the database and print them
        auto retrieved_map_with_set_pairs = key_value_db.retrieve_all<std::map, std::set>();
        print_map_with_set(retrieved_map_with_set_pairs, "Key-value pairs in database after reconcile:");

        // Inserting a new key-value pair
        key_value_db.insert(4, 3);  // Key 4 -> Set with value 3
        key_value_db.insert(4, 5);  // Add another value to the set for key 4

        // Retrieve all key-value pairs again and print them
        retrieved_map_with_set_pairs = key_value_db.retrieve_all<std::map, std::set>();
        print_map_with_set(retrieved_map_with_set_pairs, "Key-value pairs in database after inserting new values:");

    } catch (const sqlite_containers::sqlite_exception &e) {
        std::cerr << "SQLite error: " << e.what() << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
