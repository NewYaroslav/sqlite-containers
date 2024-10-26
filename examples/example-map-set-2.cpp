#include <iostream>
#include <sqlite_containers/KeyMultiValueDB.hpp>
#include <map>
#include <set>

// Structure for storing data in the database
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

    // Equality operator for use in std::unordered_map for the reconcile method
    bool operator==(const MyStruct& other) const {
        return a == other.a && b == other.b;
    }

    // Comparison operator for use in std::set
    bool operator<(const MyStruct& other) const {
        return a < other.a || (a == other.a && b < other.b);
    }
};

// std::hash specialization for MyStruct
namespace std {
    template<>
    struct hash<MyStruct> {
        std::size_t operator()(const MyStruct& ms) const {
            return std::hash<int64_t>()(ms.a) ^ std::hash<double>()(ms.b);
        }
    };
}

// Function to print the contents of a std::map where values are represented as std::set
template <typename MapType>
void print_map_with_set(const MapType& map, const std::string& header) {
    std::cout << header << std::endl;
    for (const auto& pair : map) {
        if (!pair.second.empty()) {
            for (const auto& item : pair.second) {
                std::cout << "Key: " << pair.first << " -> Struct: {" << item.a << ", " << item.b << "}" << std::endl;
            }
        } else {
            std::cout << "Key: " << pair.first << " has an empty set." << std::endl;
        }
    }
}

int main() {
    try {
        // Creating database configuration
        sqlite_containers::Config config;
        config.db_path = "example-struct-map-set.db";  // Path to the database

        // Creating a KeyMultiValueDB instance to work with int keys and MyStruct values
        sqlite_containers::KeyMultiValueDB<int, MyStruct> key_value_db(config);
        key_value_db.connect();  // Connect to the database

        // Clearing the table to start fresh
        key_value_db.clear();

        // Creating a std::map with int keys and std::set<MyStruct> values
        std::map<int, std::set<MyStruct>> map_with_set_pairs = {
            {3, {{1, 1.1}, {2, 2.2}}},
            {1, {}}   // Empty set for key 1
        };

        // Synchronizing the contents of std::map with the database
        std::cout << "Appending data to the database using reconcile..." << std::endl;
        key_value_db.reconcile(map_with_set_pairs);  // Synchronize data with the database

        // Retrieving all key-value pairs from the database and printing them
        auto retrieved_map_with_set_pairs = key_value_db.retrieve_all<std::map, std::set>();
        print_map_with_set(retrieved_map_with_set_pairs, "Key-value pairs in database after reconcile:");

        // Inserting a new key-value pair
        key_value_db.insert(4, {3, 3.3});  // Key 4 -> Set with MyStruct {3, 3.3}
        key_value_db.insert(4, {5, 5.5});  // Adding another value to the set for key 4

        // Retrieving all key-value pairs again and printing them after insertion
        retrieved_map_with_set_pairs = key_value_db.retrieve_all<std::map, std::set>();
        print_map_with_set(retrieved_map_with_set_pairs, "Key-value pairs in database after inserting new values:");

    } catch (const sqlite_containers::sqlite_exception &e) {
        std::cerr << "SQLite error: " << e.what() << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
