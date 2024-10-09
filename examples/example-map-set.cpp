#include <iostream>
#include <sqlite_containers/KeyMultiValueDB.hpp>

int main() {
    try {
        // Create database configuration
        sqlite_containers::Config config;
        config.db_path = "example-map-set.db";

        // Create KeyMultiValueDB instance
        sqlite_containers::KeyMultiValueDB<int, int> key_value_db(config);
        key_value_db.connect();

        // Clear the table for a fresh start
        key_value_db.clear();

        // Create a std::map with std::set as values
        std::map<int, std::set<int>> map_with_set_pairs = {
            {3, {1, 2}},
            {1, {}},
        };

        // Append the contents of the std::map with std::set values to the database
        //key_value_db.append(map_with_set_pairs);
        std::cout << "reconcile" << std::endl;
        key_value_db.reconcile(map_with_set_pairs);

        // Retrieve all key-value pairs from the database and print them
        auto retrieved_map_with_set_pairs = key_value_db.retrieve_all<std::map, std::set>();
        std::cout << "Key-value pairs in database after append:" << std::endl;
        for (const auto& pair : retrieved_map_with_set_pairs) {
            for (const auto& item : pair.second) {
                std::cout << pair.first << " -> " << item << std::endl;
            }
            if (pair.second.empty()) {
                std::cout << pair.first << std::endl;
            }
        }

    } catch (const sqlite_containers::sqlite_exception &e) {
        std::cerr << "SQLite error: " << e.what() << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
