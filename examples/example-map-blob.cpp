#include <sqlite_containers/KeyValueDB.hpp>
#include <iostream>
#include <map>
#include <vector>
#include <sstream>

// Helper function to print contents of std::map
template <typename MapType>
void print_map(const MapType& map, const std::string& header) {
    std::cout << header << std::endl;
    for (const auto& pair : map) {
        std::cout << "Key: " << pair.first << ", Value: ";
        for (char c : pair.second) {
            std::cout << c;
        }
        std::cout << std::endl;
    }
}

int main() {
    // Database configuration creation
    sqlite_containers::Config config;
    config.db_path = "example_vector.db";  // Path to the database

    // Creating KeyValueDB instance for int keys and std::vector<char> values
    sqlite_containers::KeyValueDB<int, std::vector<char>> map_db(config);
    map_db.connect();  // Connect to the database

    // Inserting several key-value pairs
    map_db.insert(1, {'a', 'b', 'c'});
    map_db.insert(2, {'d', 'e', 'f'});
    map_db.insert(3, {'g', 'h', 'i'});

    // Find and print the value for key 2
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

    // Load database contents into a std::map using load method
    std::map<int, std::vector<char>> my_map;
    map_db.load(my_map);
    print_map(my_map, "Contents of my_map after load:");

    // Retrieve all key-value pairs directly from the database
    std::map<int, std::vector<char>> all_entries = map_db.retrieve_all<std::map>();
    print_map(all_entries, "Contents of the database using retrieve_all:");

    // Removing a key-value pair with key 3
    map_db.remove(3);
    all_entries = map_db.retrieve_all<std::map>();
    print_map(all_entries, "Contents of the database after removing key 3:");

    // Inserting a new key-value pair
    map_db.insert(4, {'j', 'k', 'l'});
    all_entries = map_db.retrieve_all<std::map>();
    print_map(all_entries, "Contents of the database after inserting key 4:");

    // Append the contents of std::map to the database
    my_map[5] = {'m', 'n', 'o'};
    map_db.append(my_map);  // Append data from my_map to the database
    all_entries = map_db.retrieve_all<std::map>();
    print_map(all_entries, "Contents of the database after append:");

    // Example of reconciling (syncing) std::map contents with the database
    my_map.erase(5);  // Remove a key-value pair from the map
    my_map[6] = {'p', 'q', 'r'};
    map_db.reconcile(my_map);  // Synchronize database with map changes
    all_entries = map_db.retrieve_all<std::map>();
    print_map(all_entries, "Contents of the database after reconcile:");

    // Using assignment operator for synchronization (same as reconcile)
    my_map.erase(6);  // Remove a key-value pair from the map
    my_map[7] = {'s', 't', 'u'};
    map_db = my_map;  // Use assignment operator to sync with the database
    all_entries = map_db.retrieve_all<std::map>();
    print_map(all_entries, "Contents of the database after using operator= to reconcile:");

    // Clearing the database
    map_db.clear();
    all_entries = map_db.retrieve_all<std::map>();
    if (all_entries.empty()) {
        std::cout << "Database is empty after clear." << std::endl;
    } else {
        print_map(all_entries, "Contents of the database after clear:");
    }

    return 0;
}
