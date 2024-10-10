#include <sqlite_containers/KeyMultiValueDB.hpp>
#include <iostream>
#include <map>
#include <list>

// Structure to be used as a value in the database
struct MyStruct {
    int64_t a;
    double b;

    // Operators for serialization and deserialization of the structure
    friend std::ostream& operator<<(std::ostream& os, const MyStruct& ms) {
        os << ms.a << " " << ms.b;
        return os;
    }

    friend std::istream& operator>>(std::istream& is, MyStruct& ms) {
        is >> ms.a >> ms.b;
        return is;
    }
};

// Function to print the contents of a multimap
template <typename MultimapType>
void print_multimap(const MultimapType& multimap, const std::string& header) {
    std::cout << header << std::endl;
    for (const auto& pair : multimap) {
        std::cout << "Key: " << pair.first << " -> Struct: {" << pair.second.a << ", " << pair.second.b << "}" << std::endl;
    }
}

// Function to print the contents of a map where values are lists
template <typename MapType>
void print_map_with_list(const MapType& map, const std::string& header) {
    std::cout << header << std::endl;
    for (const auto& pair : map) {
        std::cout << "Key: " << pair.first << " -> ";
        for (const auto& item : pair.second) {
            std::cout << "Struct: {" << item.a << ", " << item.b << "} ";
        }
        std::cout << std::endl;
    }
}

int main() {
    try {
        // Creating a configuration for the database
        sqlite_containers::Config config;
        config.db_path = "example-multimap-struct.db";

        // Creating an instance of KeyMultiValueDB
        sqlite_containers::KeyMultiValueDB<int, MyStruct> key_value_db(config);
        key_value_db.connect();

        // Clearing the table for a fresh start
        key_value_db.clear();

        // Creating a std::multimap with key-value pairs (structures)
        std::multimap<int, MyStruct> multimap_pairs = {
            {1, {10, 1.1}},
            {2, {20, 2.2}},
            {2, {20, 2.2}},   // Duplicate values
            {2, {20, 2.2}},   // Another duplicate
            {1, {14, 4.0}},
            {3, {15, 4.0}},
            {2, {30, 1.3}}
        };

        // Creating a std::map with std::list as values
        std::map<int, std::list<MyStruct>> map_with_list_pairs = {
            {3, {{15, 4.0}}},
            {1, {{10, 1.1}}}
        };

        // Adding the contents of std::multimap to the database
        key_value_db.append(multimap_pairs);

        // Adding the contents of std::map with std::list values to the database
        key_value_db.append(map_with_list_pairs);

        // Retrieving all key-value pairs from the database and printing them
        auto retrieved_key_value_pairs = key_value_db.retrieve_all<std::multimap>();
        print_multimap(retrieved_key_value_pairs, "Key-value pairs in database after append:");

        // Inserting a new key-value pair
        key_value_db.insert(4, {50, 7.7});
        key_value_db.insert(4, {50, 7.7});

        // Retrieving all key-value pairs into a std::map with std::list values and printing them
        using map_with_list_t = std::map<int, std::list<MyStruct>>;
        map_with_list_t retrieved_map_with_list_pairs = key_value_db.retrieve_all<std::map, std::list>();
        print_map_with_list(retrieved_map_with_list_pairs, "Key-value pairs in database after insert:");

        // Checking the existence of a key in the database
        std::list<MyStruct> values;
        if (key_value_db.find(4, values)) {
            std::cout << "Key 4 found in the database with values:" << std::endl;
            for (const auto& value : values) {
                std::cout << "Struct: {" << value.a << ", " << value.b << "}" << std::endl;
            }
        } else {
            std::cout << "Key 4 not found in the database." << std::endl;
        }

        // Checking a non-existent key
        values.clear();
        if (key_value_db.find(10, values)) {
            std::cout << "Key 10 found in the database with values:" << std::endl;
            for (const auto& value : values) {
                std::cout << "Struct: {" << value.a << ", " << value.b << "}" << std::endl;
            }
        } else {
            std::cout << "Key 10 not found in the database." << std::endl;
        }

        // Removing a specific key-value pair
        key_value_db.remove(2, {20, 2.2});

        // Removing all values associated with a key
        key_value_db.remove(1);

        // Retrieving all key-value pairs after removal and printing them
        retrieved_key_value_pairs = key_value_db.retrieve_all<std::multimap>();
        print_multimap(retrieved_key_value_pairs, "Key-value pairs in database after removals:");

        // Synchronizing the database with the contents of std::multimap
        key_value_db.reconcile(multimap_pairs);

        // Retrieving all key-value pairs after synchronization and printing them
        retrieved_key_value_pairs = key_value_db.retrieve_all<std::multimap>();
        print_multimap(retrieved_key_value_pairs, "Key-value pairs in database after reconcile:");

    } catch (const sqlite_containers::sqlite_exception &e) {
        std::cerr << "SQLite error: " << e.what() << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
