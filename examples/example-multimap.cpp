#include <sqlite_containers/KeyMultiValueDB.hpp>
#include <iostream>

int main() {
    try {
        // Create database configuration
        sqlite_containers::Config config;
        config.db_path = "example-multimap.db";

        // Create KeyMultiValueDB instance
        sqlite_containers::KeyMultiValueDB<int, std::string> key_value_db(config);
        key_value_db.connect();

        // Clear the table for a fresh start
        key_value_db.clear();

        // Create a std::multimap with key-value pairs
        std::multimap<int, std::string> multimap_pairs = {
            {1, "apple"},
            {2, "banana"},
            {2, "banana"},
            {1, "apricot"},
            {3, "cherry"},
            {2, "blueberry"}
        };

        // Create a std::map with std::set as values
        std::map<int, std::set<std::string>> map_with_set_pairs = {
            {3, {"cherry"}},
            {1, {"banana"}},
        };

        // Append the contents of the std::multimap to the database
        key_value_db.append(multimap_pairs);

        // Append the contents of the std::map with std::set values to the database
        key_value_db.append(map_with_set_pairs);

        // Retrieve all key-value pairs from the database and print them
        auto retrieved_key_value_pairs = key_value_db.retrieve_all<std::multimap>();
        std::cout << "Key-value pairs in database after append:" << std::endl;
        for (const auto& pair : retrieved_key_value_pairs) {
            std::cout << pair.first << " -> " << pair.second << std::endl;
        }

        // Insert a new key-value pair
        key_value_db.insert(4, "date");
        key_value_db.insert(4, "date");

        // Retrieve all key-value pairs from the database into a std::map with std::list values and print them
        using map_with_list_t = std::map<int, std::list<std::string>>;
        map_with_list_t retrieved_map_with_list_pairs = key_value_db.retrieve_all<std::map, std::list>();
        std::cout << "Key-value pairs in database after insert:" << std::endl;
        for (const auto& pair : retrieved_map_with_list_pairs) {
            for (const auto& item : pair.second) {
                std::cout << pair.first << " -> " << item << std::endl;
            }
        }

        // Check if the key exists in the database
        std::list<std::string> values; // or std::vector<std::string>, std::set<std::string>
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
            std::cout << "Key 10 found in the database with values: ";
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
        std::cout << "Key-value pairs in database after removals:" << std::endl;
        for (const auto& pair : retrieved_key_value_pairs) {
            std::cout << pair.first << " -> " << pair.second << std::endl;
        }

        // Reconcile the database with the contents of the std::multimap
        key_value_db.reconcile(multimap_pairs);

        // Retrieve all key-value pairs from the database after reconcile and print them
        retrieved_key_value_pairs = key_value_db.retrieve_all<std::multimap>();
        std::cout << "Key-value pairs in database after reconcile:" << std::endl;
        for (const auto& pair : retrieved_key_value_pairs) {
            std::cout << pair.first << " -> " << pair.second << std::endl;
        }

    } catch (const sqlite_containers::sqlite_exception &e) {
        std::cerr << "SQLite error: " << e.what() << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
