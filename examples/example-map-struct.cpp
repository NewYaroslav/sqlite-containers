#include <sqlite_containers/KeyValueDB.hpp>
#include <iostream>
#include <map>
#include <vector>

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
};

int main() {
    // Database configuration creation
    sqlite_containers::Config config;
    config.db_path = "example_struct.db";

    // Creation of KeyValueDB instance
    sqlite_containers::KeyValueDB<int, MyStruct> map_db(config);
    map_db.connect();

    // Inserting several key-value pairs
    map_db.insert(1, {10, 1.1});
    map_db.insert(2, {20, 2.2});
    map_db.insert(3, {30, 3.3});

    // Finding and printing a value
    MyStruct value;
    if (map_db.find(2, value)) {
        std::cout << "Found value for key 2: " << value << std::endl;
    } else {
        std::cout << "Key 2 not found." << std::endl;
    }

    // Load the database contents into a std::map
    std::map<int, MyStruct> my_map;
    map_db.load(my_map);

    // Printing all key-value pairs
    std::cout << "Contents of my_map after load:" << std::endl;
    for (const auto& pair : my_map) {
        std::cout << "Key: " << pair.first << ", Value: " << pair.second << std::endl;
    }

    // Retrieving all key-value pairs directly from the database
    std::map<int, MyStruct> all_entries = map_db.retrieve_all<std::map>();
    std::cout << "Contents of the database using retrieve_all:" << std::endl;
    for (const auto& pair : all_entries) {
        std::cout << "Key: " << pair.first << ", Value: " << pair.second << std::endl;
    }

    // Removing a key-value pair
    map_db.remove(3);
    all_entries = map_db.retrieve_all<std::map>();
    std::cout << "Contents of the database after removing key 3:" << std::endl;
    for (const auto& pair : all_entries) {
        std::cout << "Key: " << pair.first << ", Value: " << pair.second << std::endl;
    }

    // Inserting a new key-value pair
    map_db.insert(4, {40, 4.4});
    all_entries = map_db.retrieve_all<std::map>();
    std::cout << "Contents of the database after inserting key 4:" << std::endl;
    for (const auto& pair : all_entries) {
        std::cout << "Key: " << pair.first << ", Value: " << pair.second << std::endl;
    }

    // Append the contents of the std::map to the database
    my_map[5] = {50, 5.5};
    map_db.append(my_map);

    all_entries = map_db.retrieve_all<std::map>();
    std::cout << "Contents of the database after append:" << std::endl;
    for (const auto& pair : all_entries) {
        std::cout << "Key: " << pair.first << ", Value: " << pair.second << std::endl;
    }

    // Clearing the database
    map_db.clear();
    all_entries = map_db.retrieve_all<std::map>();
    std::cout << "Contents of the database after clear:" << std::endl;
    if (all_entries.empty()) {
        std::cout << "Database is empty." << std::endl;
    } else {
        for (const auto& pair : all_entries) {
            std::cout << "Key: " << pair.first << ", Value: " << pair.second << std::endl;
        }
    }

    return 0;
}
