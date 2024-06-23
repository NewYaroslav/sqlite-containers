#include <sqlite_containers/MapDB.hpp>
#include <iostream>
#include <map>
#include <vector>

struct MyStruct {
    int a;
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
};

int main() {
    // Database configuration creation
    sqlite_containers::Config config;
    config.db_path = "example_struct.db";

    // Creation of MapDB instance
    sqlite_containers::MapDB<int, MyStruct> map_db(config);
    map_db.connect();

    // Inserting several key-value pairs
    map_db.insert(1, {10, 1.1});
    map_db.insert(2, {20, 2.2});
    map_db.insert(3, {30, 3.3});

    // Finding and printing a value
    MyStruct value;
    if (map_db.find(2, value)) {
        std::cout << "Found value for key 2: " << value.a << ", " << value.b << std::endl;
    } else {
        std::cout << "Key 2 not found." << std::endl;
    }

    // Synchronizing the database with std::map
    std::map<int, MyStruct> my_map;
    map_db.sync_to_map(my_map);

    // Printing all key-value pairs
    std::cout << "Contents of my_map after sync_to_map:" << std::endl;
    for (const auto& pair : my_map) {
        std::cout << "Key: " << pair.first << ", Value: " << pair.second.a << ", " << pair.second.b << std::endl;
    }

    // Retrieving all key-value pairs directly from the database
    std::map<int, MyStruct> all_entries = map_db.retrieve_all<std::map>();
    std::cout << "Contents of the database using retrieve_all:" << std::endl;
    for (const auto& pair : all_entries) {
        std::cout << "Key: " << pair.first << ", Value: " << pair.second.a << ", " << pair.second.b << std::endl;
    }

    // Removing a key-value pair
    map_db.remove(3);
    all_entries = map_db.retrieve_all<std::map>();
    std::cout << "Contents of the database after removing key 3:" << std::endl;
    for (const auto& pair : all_entries) {
        std::cout << "Key: " << pair.first << ", Value: " << pair.second.a << ", " << pair.second.b << std::endl;
    }

    // Inserting a new key-value pair
    map_db.insert(4, {40, 4.4});
    all_entries = map_db.retrieve_all<std::map>();
    std::cout << "Contents of the database after inserting key 4:" << std::endl;
    for (const auto& pair : all_entries) {
        std::cout << "Key: " << pair.first << ", Value: " << pair.second.a << ", " << pair.second.b << std::endl;
    }

    // Synchronizing std::map with the database
    my_map[5] = {50, 5.5};
    map_db.sync_to_db(my_map);

    all_entries = map_db.retrieve_all<std::map>();
    std::cout << "Contents of the database after sync_to_db:" << std::endl;
    for (const auto& pair : all_entries) {
        std::cout << "Key: " << pair.first << ", Value: " << pair.second.a << ", " << pair.second.b << std::endl;
    }

    // Clearing the database
    map_db.clear();
    all_entries = map_db.retrieve_all<std::map>();
    std::cout << "Contents of the database after clear:" << std::endl;
    if (all_entries.empty()) {
        std::cout << "Database is empty." << std::endl;
    } else {
        for (const auto& pair : all_entries) {
            std::cout << "Key: " << pair.first << ", Value: " << pair.second.a << ", " << pair.second.b << std::endl;
        }
    }

    return 0;
}
