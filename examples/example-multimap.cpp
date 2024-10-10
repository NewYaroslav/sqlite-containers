#include <sqlite_containers/KeyMultiValueDB.hpp>
#include <iostream>
#include <map>
#include <set>
#include <list>

// Utility function to print contents of a multimap
template <typename MultimapType>
void print_multimap(const MultimapType& multimap, const std::string& header) {
    std::cout << header << std::endl;
    for (const auto& pair : multimap) {
        std::cout << "Key: " << pair.first << ", Value: " << pair.second << std::endl;
    }
}

// Utility function to print contents of a map with list as values
template <typename MapType>
void print_map_with_list(const MapType& map, const std::string& header) {
    std::cout << header << std::endl;
    for (const auto& pair : map) {
        for (const auto& item : pair.second) {
            std::cout << "Key: " << pair.first << ", Value: " << item << std::endl;
        }
    }
}

int main() {
    try {
        // Create a configuration for the database
        sqlite_containers::Config config;
        config.db_path = "example-multimap.db";

        // Create a KeyMultiValueDB instance
        sqlite_containers::KeyMultiValueDB<int, std::string> key_value_db(config);
        key_value_db.connect();

        // Clear the table for a fresh start
        key_value_db.clear();

        // Create a std::multimap with key-value pairs
        std::multimap<int, std::string> multimap_pairs = {
            {1, "apple"},
            {2, "banana"},
            {2, "banana"},   // Duplicate key-value pair
            {1, "apricot"},
            {3, "cherry"},
            {2, "blueberry"}
        };

        // Create a std::map with std::set as values
        std::map<int, std::set<std::string>> map_with_set_pairs = {
            {3, {"cherry"}},    // Key 3 maps to a set with "cherry"
            {1, {"banana"}},    // Key 1 maps to a set with "banana"
            {4, {}},            // Key 4 with an empty set, will be ignored during insertion
        };

        // Append the contents of the std::multimap to the database
        key_value_db.append(multimap_pairs);

        // Append the contents of the std::map with std::set values to the database
        key_value_db.append(map_with_set_pairs);

        // Retrieve all key-value pairs from the database and print them
        auto retrieved_key_value_pairs = key_value_db.retrieve_all<std::multimap>();
        print_multimap(retrieved_key_value_pairs, "Key-value pairs in database after append:");

        // Use operator= to assign the std::multimap to the database
        key_value_db = multimap_pairs;

        // Retrieve all key-value pairs from the database and print them
        retrieved_key_value_pairs = key_value_db.retrieve_all<std::multimap>();
        print_multimap(retrieved_key_value_pairs, "Key-value pairs in database after operator= assignment:");

        // Use operator= to assign the std::map with std::set values to the database
        key_value_db = map_with_set_pairs;

        // Retrieve all key-value pairs from the database and print them
        retrieved_key_value_pairs = key_value_db.retrieve_all<std::multimap>();
        print_multimap(retrieved_key_value_pairs, "Key-value pairs in database after operator= assignment with map:");

        // Insert a new key-value pair
        key_value_db.insert(4, "date");
        key_value_db.insert(4, "date");

        // Retrieve all key-value pairs from the database into a std::map with std::list values and print them
        using map_with_list_t = std::map<int, std::list<std::string>>;
        map_with_list_t retrieved_map_with_list_pairs = key_value_db.retrieve_all<std::map, std::list>();
        print_map_with_list(retrieved_map_with_list_pairs, "Key-value pairs in database after insert:");

        // Check if the key exists in the database
        std::list<std::string> values;
        if (key_value_db.find(4, values)) {
            std::cout << "Key 4 found in the database with values:" << std::endl;
            for (const auto& value : values) {
                std::cout << value << std::endl;
            }
        } else {
            std::cout << "Key 4 not found in the database." << std::endl;
        }

        // Check if a non-existing key is found
        values.clear();
        if (key_value_db.find(10, values)) {
            std::cout << "Key 10 found in the database with values:" << std::endl;
            for (const auto& value : values) {
                std::cout << value << " ";
            }
            std::cout << std::endl;
        } else {
            std::cout << "Key 10 not found in the database." << std::endl;
        }

        // Remove a specific key-value pair
        key_value_db.remove(2, "banana");

        // Remove all values associated with a key
        key_value_db.remove(1);

        // Retrieve all key-value pairs from the database after removal and print them
        retrieved_key_value_pairs = key_value_db.retrieve_all<std::multimap>();
        print_multimap(retrieved_key_value_pairs, "Key-value pairs in database after removals:");

        // Reconcile the database with the contents of the std::multimap
        key_value_db.reconcile(multimap_pairs);

        // Retrieve all key-value pairs from the database after reconciliation and print them
        retrieved_key_value_pairs = key_value_db();
        print_multimap(retrieved_key_value_pairs, "Key-value pairs in database after reconcile:");

        // Reconcile the database with the contents of the std::map with sets using operator=
        key_value_db = map_with_set_pairs;

        // Retrieve all key-value pairs from the database after reconciliation and print them
        retrieved_key_value_pairs = key_value_db.retrieve_all<std::multimap>();
        print_multimap(retrieved_key_value_pairs, "Key-value pairs in database after operator= reconciliation:");

        // Check the number of keys in the database
        std::cout << "Number of keys in the database: " << key_value_db.count() << std::endl;

        // Check if the database is empty
        std::cout << "Is the database empty? " << (key_value_db.empty() ? "Yes" : "No") << std::endl;

    } catch (const sqlite_containers::sqlite_exception &e) {
        std::cerr << "SQLite error: " << e.what() << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}

