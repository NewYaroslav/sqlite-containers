#include <sqlite_containers/KeyDB.hpp>
#include <sqlite_containers/KeyValueDB.hpp>
#include <sqlite_containers/KeyMultiValueDB.hpp>
#include <iostream>
#include <set>
#include <map>
#include <vector>

int main() {
    try {
        // Config for the first database with integer keys (KeyDB)
        sqlite_containers::Config config1;
        config1.db_path = "example_multi_type.db";
        config1.table_name = "integer_keys";
        sqlite_containers::KeyDB<int> key_db(config1);
        key_db.connect();

        // Config for the second database with string keys and double values (KeyValueDB)
        sqlite_containers::Config config2;
        config2.db_path = "example_multi_type.db";
        config2.table_name = "string_to_double";
        sqlite_containers::KeyValueDB<std::string, double> kv_db(config2);
        kv_db.connect();

        // Config for the third database with float keys and std::string values (KeyValueDB)
        sqlite_containers::Config config3;
        config3.db_path = "example_multi_type.db";
        config3.table_name = "float_to_string";
        sqlite_containers::KeyValueDB<float, std::string> kv_db2(config3);
        kv_db2.connect();

        // Config for the fourth database with int keys and multiple string values (KeyMultiValueDB)
        sqlite_containers::Config config4;
        config4.db_path = "example_multi_type.db";
        config4.table_name = "int_to_multi_strings";
        sqlite_containers::KeyMultiValueDB<int, std::string> kmv_db(config4);
        kmv_db.connect();

        // Clear tables for a fresh start
        key_db.clear();
        kv_db.clear();
        kv_db2.clear();
        kmv_db.clear();

        // Insert data into KeyDB
        std::set<int> int_keys = {1, 2, 3, 4};
        key_db.append(int_keys);

        // Insert data into the first KeyValueDB (string to double)
        kv_db.insert("apple", 1.1);
        kv_db.insert("banana", 2.2);
        kv_db.insert("orange", 3.3);

        // Insert data into the second KeyValueDB (float to string)
        kv_db2.insert(1.5f, "one point five");
        kv_db2.insert(2.7f, "two point seven");

        // Insert data into KeyMultiValueDB (int to multiple strings)
        kmv_db.insert(1, "value1");
        kmv_db.insert(1, "value2");
        kmv_db.insert(2, "valueA");
        kmv_db.insert(2, "valueB");

        // Retrieve and print data from KeyDB
        std::set<int> retrieved_int_keys = key_db.retrieve_all<std::set>();
        std::cout << "Keys in KeyDB: ";
        for (const auto& key : retrieved_int_keys) {
            std::cout << key << " ";
        }
        std::cout << std::endl;

        // Retrieve and print data from the first KeyValueDB
        std::map<std::string, double> string_to_double = kv_db.retrieve_all<std::map>();
        std::cout << "String-to-Double KeyValueDB contents:" << std::endl;
        for (const auto& pair : string_to_double) {
            std::cout << "Key: " << pair.first << ", Value: " << pair.second << std::endl;
        }

        // Retrieve and print data from the second KeyValueDB
        std::map<float, std::string> float_to_string = kv_db2.retrieve_all<std::map>();
        std::cout << "Float-to-String KeyValueDB contents:" << std::endl;
        for (const auto& pair : float_to_string) {
            std::cout << "Key: " << pair.first << ", Value: " << pair.second << std::endl;
        }

        // Retrieve and print data from KeyMultiValueDB
        std::multimap<int, std::string> int_to_multi_strings = kmv_db.retrieve_all<std::multimap>();
        std::cout << "Int-to-MultiStrings KeyMultiValueDB contents:" << std::endl;
        for (const auto& pair : int_to_multi_strings) {
            std::cout << "Key: " << pair.first << ", Value: " << pair.second << std::endl;
        }

    } catch (const sqlite_containers::sqlite_exception& e) {
        std::cerr << "SQLite error: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}

