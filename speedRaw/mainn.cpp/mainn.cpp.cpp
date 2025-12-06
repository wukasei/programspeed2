#include <iostream>
#include <jdbc/mysql_driver.h>
#include <jdbc/mysql_connection.h>
#include <jdbc/cppconn/statement.h>
#include <jdbc/cppconn/resultset.h>
#include <memory>
#include <windows.h>
#include <chrono>
#include "raw.cpp"

int main() {

    SetConsoleOutputCP(CP_UTF8);

    auto con = connectToDatabase();
    if (!con) return 1;

    std::vector<std::string> tables = { "client", "driver", "vehicle", "`order`", "tripdetails", "triplog"};

    while (true) {
        std::cout << "\n=== Database Performance Menu ===\n";
        std::cout << "1. Test SELECT on a single table\n";
        std::cout << "2. Test JOIN query average\n";
        std::cout << "3. Test INSERT/UPDATE/DELETE average\n";
        std::cout << "0. Exit\n";
        std::cout << "Enter your choice: ";

        int choice;
        std::cin >> choice;

        if (choice == 0) break;

        switch (choice) {
        case 1: {

            std::cout << "Enter limit: ";
            int limit;
            std::cin >> limit;

            std::cout << "Enter reapets: ";
            int reapets;
            std::cin >> reapets;

            std::vector<std::string> allTables = {
                "client", "driver", "vehicle", "`order`",
                "tripdetails", "triplog"
            };

            averageSelectAllTables(limit, reapets, con.get(), allTables);

            break;
        }
        case 2: {
            std::cout << "Enter JOIN type (INNER, LEFT, RIGHT): ";
            std::string type;
            std::cin >> type;

            std::cout << "Enter limit: ";
            int limit;
            std::cin >> limit;

            std::cout << "Enter number of repeats: ";
            int repeats;
            std::cin >> repeats;

            averageJoinTime(type, limit, repeats, con.get());
            break;
        }
        case 3: {
            std::cout << "Enter table name (client, driver, vehicle, `order`, tripdetails, triplog): ";
            std::string table;
            std::cin >> table;

            std::cout << "Enter number of rows to insert: ";
            int count;
            std::cin >> count;

            try {
                con->setAutoCommit(false);  

                std::cout << "Inserting " << count << " rows into " << table << "...\n";
                auto insertedIds = averageInsertRaw(con.get(), table, count);

                std::cout << "Updating inserted rows...\n";
                averageUpdateTime(con.get(), table, insertedIds, 100);

                std::cout << "Deleting inserted rows...\n";
                averageDeleteTime(con.get(), table, insertedIds);

                con->rollback();  
                std::cout << "Transaction rolled back. Database unchanged.\n";
            }
            catch (sql::SQLException& e) {
                con->rollback();
                std::cerr << "Transaction error, rolled back: " << e.what() << std::endl;
            }

            break;
          
        }
        default:
            std::cout << "Invalid choice. Try again.\n";
            break;
        }
    }

    std::cout << "Exiting program.\n";
    return 0;
    

    // Set console output to UTF-8
    //SetConsoleOutputCP(CP_UTF8);

    //try {
    //    sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();

    //    // Connect to MySQL (classic API, port 3306)
    //    std::unique_ptr<sql::Connection> con(
    //        driver->connect("tcp://127.0.0.1:3306", "root", "15387241")
    //    );

    //    std::cout << "Connected successfully!\n";

    //    con->setSchema("transportcompany");

    //    // Create a single statement object
    //    std::unique_ptr<sql::Statement> stmt(con->createStatement());

    //    // Ensure connection sends/receives UTF-8
    //    stmt->execute("SET NAMES utf8mb4");

    //    // Execute query
    //    std::unique_ptr<sql::ResultSet> res(stmt->executeQuery("SELECT * FROM driver"));

    //    while (res->next()) {
    //        std::cout << res->getInt("driver_id") << " - "
    //            << res->getString("first_name") << std::endl;
    //    }

    //}
    //catch (sql::SQLException& e) {
    //    std::cerr << "MySQL Error: " << e.what() << std::endl;
    //    return 1;
    //}
}
