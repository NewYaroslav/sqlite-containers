#include <sqlite_containers/KeyValueDB.hpp>
#include <iostream>
#include <map>

// Utility function to print contents of the map
template <typename MapType>
void print_map(const MapType& map, const std::string& header) {
    std::cout << header << std::endl;
    for (const auto& pair : map) {
        std::cout << "Key: " << pair.first << ", Value: " << pair.second << std::endl;
    }
}

int main() {
    // Create a configuration for the database
    sqlite_containers::Config config;
    config.db_path = "example-map.db";

    // Create a KeyValueDB instance
    sqlite_containers::KeyValueDB<int, std::string> map_db(config);
    map_db.connect();

    // Insert key-value pairs
    map_db.insert(1, "value1");
    map_db.insert(2, "value2");
    map_db.insert(3, "value3");

    // Find and print a value by key
    std::string value;
    if (map_db.find(2, value)) {
        std::cout << "Found value for key 2: " << value << std::endl;
    } else {
        std::cout << "Key 2 not found." << std::endl;
    }

    // Load the database contents into a std::map using the overloaded () operator
    std::map<int, std::string> my_map;
    my_map = map_db();  // Use the overloaded () operator to load data from the database into the map
    print_map(my_map, "Contents of my_map after using operator():");

    // Load the database contents into another std::map using the load method
    std::map<int, std::string> my_map2;
    map_db.load(my_map2);  // Use the load method to explicitly load data from the database
    print_map(my_map2, "Contents of my_map2 after using load:");

    // Retrieve and print all key-value pairs directly from the database
    std::map<int, std::string> all_entries = map_db.retrieve_all<std::map>();
    print_map(all_entries, "Contents of database using retrieve_all:");

    // Remove a key-value pair and print the result
    map_db.remove(3);
    all_entries = map_db.retrieve_all<std::map>();
    print_map(all_entries, "Contents of database after removing key 3:");

    // Insert a new key-value pair and print the result
    map_db.insert(4, "value4");
    all_entries = map_db.retrieve_all<std::map>();
    print_map(all_entries, "Contents of database after inserting key 4:");

    // Append the contents of the std::map to the database and print the result
    my_map[5] = "value5";
    map_db.append(my_map);
    all_entries = map_db.retrieve_all<std::map>();
    print_map(all_entries, "Contents of database after append:");

    // Reconcile the contents of the std::map with the database and print the result
    my_map.erase(5);
    my_map[6] = "value6";
    map_db.reconcile(my_map);
    all_entries = map_db.retrieve_all<std::map>();
    print_map(all_entries, "Contents of database after reconcile:");

    // Use operator= to reconcile (equivalent to calling reconcile)
    my_map.erase(6);
    my_map[7] = "value7";
    map_db = my_map;  // Operator= to append or reconcile the contents of the map to the database
    all_entries = map_db.retrieve_all<std::map>();
    print_map(all_entries, "Contents of database after using operator= to reconcile:");

    // Print count and empty status of the database
    std::cout << "count: " << map_db.count() << std::endl;
    std::cout << "empty: " << map_db.empty() << std::endl;

    // Clear the database and print the result
    map_db.clear();
    all_entries = map_db.retrieve_all<std::map>();
    if (all_entries.empty()) {
        std::cout << "Database is empty after clear." << std::endl;
    } else {
        print_map(all_entries, "Contents of database after clear:");
    }

    // Print the final empty status
    std::cout << "empty: " << map_db.empty() << std::endl;

    return 0;
}
