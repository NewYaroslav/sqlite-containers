#include <sqlite_containers/KeyValueDB.hpp>
#include <iostream>
#include <map>
#include <vector>

// Data structure for storing in the database
struct MyStruct {
    int64_t a;
    double b;

    // Overloading the output operator for serializing the structure
    friend std::ostream& operator<<(std::ostream& os, const MyStruct& ms) {
        os << ms.a << " " << ms.b;
        return os;
    }

    // Overloading the input operator for deserializing the structure
    friend std::istream& operator>>(std::istream& is, MyStruct& ms) {
        is >> ms.a >> ms.b;
        return is;
    }
};

// Helper function to print the contents of a std::map
template <typename MapType>
void print_map(const MapType& map, const std::string& header) {
    std::cout << header << std::endl;
    for (const auto& pair : map) {
        std::cout << "Key: " << pair.first << ", Value: " << pair.second << std::endl;
    }
}

int main() {
    // Creating a database configuration
    sqlite_containers::Config config;
    config.db_path = "example_struct.db";  // Specify the database path

    // Creating a database instance for working with int keys and MyStruct values
    sqlite_containers::KeyValueDB<int, MyStruct> map_db(config);
    map_db.connect();  // Connect to the database

    // Inserting several key-value pairs
    map_db.insert(1, {10, 1.1});
    map_db.insert(2, {20, 2.2});
    map_db.insert(3, {30, 3.3});

    // Find and output the value for key 2
    MyStruct value;
    if (map_db.find(2, value)) {
        std::cout << "Found value for key 2: " << value << std::endl;
    } else {
        std::cout << "Key 2 not found." << std::endl;
    }

    // Load database contents into a std::map using overloaded operator()
    std::map<int, MyStruct> my_map;
    my_map = map_db();  // Use overloaded operator() to load data
    print_map(my_map, "Contents of my_map after using operator():");

    // Load database contents into another std::map using the load method
    std::map<int, MyStruct> my_map2;
    map_db.load(my_map2);  // Use the load method for explicit data loading
    print_map(my_map2, "Contents of my_map2 after using load:");

    // Retrieve and output all key-value pairs directly from the database
    std::map<int, MyStruct> all_entries = map_db.retrieve_all<std::map>();
    print_map(all_entries, "Contents of database using retrieve_all:");

    // Remove key-value pair with key 3 and output the result
    map_db.remove(3);
    all_entries = map_db.retrieve_all<std::map>();
    print_map(all_entries, "Contents of database after removing key 3:");

    // Insert a new key-value pair and output the result
    map_db.insert(4, {40, 4.4});
    all_entries = map_db.retrieve_all<std::map>();
    print_map(all_entries, "Contents of database after inserting key 4:");

    // Add the contents of a std::map to the database using the append method
    my_map[5] = {50, 5.5};
    map_db.append(my_map);  // Append data from my_map to the database

    all_entries = map_db.retrieve_all<std::map>();
    print_map(all_entries, "Contents of database after append:");

    // Example of synchronizing the contents of a std::map with the database using the reconcile method
    my_map.erase(5);
    my_map[6] = {60, 6.6};
    map_db.reconcile(my_map);  // Synchronize data
    all_entries = map_db.retrieve_all<std::map>();
    print_map(all_entries, "Contents of database after reconcile:");

    // Using the assignment operator for synchronization (similar to calling the reconcile method)
    my_map.erase(6);
    my_map[7] = {70, 7.7};
    map_db = my_map;  // Use the assignment operator for synchronization
    all_entries = map_db.retrieve_all<std::map>();
    print_map(all_entries, "Contents of database after using operator= to reconcile:");

    // Output the number of elements and whether the database is empty
    std::cout << "count: " << map_db.count() << std::endl;
    std::cout << "empty: " << map_db.empty() << std::endl;

    // Clear the database and output the result
    map_db.clear();
    all_entries = map_db.retrieve_all<std::map>();
    if (all_entries.empty()) {
        std::cout << "Database is empty after clear." << std::endl;
    } else {
        print_map(all_entries, "Contents of database after clear:");
    }

    // Final check for emptiness
    std::cout << "empty: " << map_db.empty() << std::endl;

    return 0;
}

