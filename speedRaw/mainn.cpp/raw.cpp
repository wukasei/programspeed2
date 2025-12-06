#include "raw.h"
#include <iostream>
#include <chrono>
#include <numeric>
#include <vector>
#include <sstream>
#include <random>
#include <memory>

#include <fstream>  
#include "json.hpp"
using json = nlohmann::json;


#include <jdbc/mysql_driver.h>
#include <jdbc/mysql_connection.h>
#include <jdbc/cppconn/statement.h>
#include <jdbc/cppconn/resultset.h>



std::unique_ptr<sql::Connection> connectToDatabase() {
    try {
        sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
        std::unique_ptr<sql::Connection> con(driver->connect("tcp://127.0.0.1:3306", "root", "15387241"));
        con->setSchema("transportcompany");

        std::unique_ptr<sql::Statement> stmt(con->createStatement());
        stmt->execute("SET NAMES utf8mb4");

        std::cout << " Connected to database 'transportcompany' successfully!\n";
        return con;
    }
    catch (sql::SQLException& e) {
        std::cerr << " Connection error: " << e.what() << std::endl;
        return nullptr;
    }
}

//Select


double testRawSelectTime(const std::string& table, int limit, sql::Connection* con) {
    try {
        auto start = std::chrono::high_resolution_clock::now();

        std::unique_ptr<sql::Statement> stmt(con->createStatement());
        std::string query = "SELECT * FROM " + table + " LIMIT " + std::to_string(limit);

        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery(query));

        int rows = 0;
        while (res->next()) rows++;

        auto end = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(end - start).count();

        std::cout << " SELECT " << table << " | LIMIT " << limit
            << " | Time: " << ms << " ms (" << rows << " rows)\n";

        return ms;
    }
    catch (sql::SQLException& e) {
        std::cerr << " Query error: " << e.what() << std::endl;
        return 0.0;
    }
}


void averageSelectAllTables(int limit, int repeats, sql::Connection* con,
    const std::vector<std::string>& tables)
{
    std::cout << "\n Average SELECT tests (" << repeats
        << " repeats, LIMIT " << limit << "):\n";

    struct Result {
        std::string table;
        double avg;
        double min;
        double max;
    };

    std::vector<Result> results;

    for (const auto& table : tables) {
        std::vector<double> times;
        times.reserve(repeats);

        for (int i = 0; i < repeats; i++) {
            times.push_back(testRawSelectTime(table, limit, con));
        }

        double sum = std::accumulate(times.begin(), times.end(), 0.0);
        double avg = sum / times.size();
        double min = *std::min_element(times.begin(), times.end());
        double max = *std::max_element(times.begin(), times.end());

        results.push_back({ table, avg, min, max });

        std::cout << "\n" << table << " results:\n";
        std::cout << "  Average: " << avg << " ms\n";
        std::cout << "  Minimum: " << min << " ms\n";
        std::cout << "  Maximum: " << max << " ms\n";
    }

    std::cout << "\n Summary SELECT:\n";
    for (const auto& r : results) {
        std::cout << r.table
            << std::string(12 - r.table.size(), ' ')
            << " | Avg: " << r.avg
            << " ms | Min: " << r.min
            << " ms | Max: " << r.max
            << " ms\n";
    }
}


//Join

double testRawJoin(const std::string& type, int limit, sql::Connection* con) {
    if (limit <= 0) limit = 10;

    try {
        auto start = std::chrono::high_resolution_clock::now();

        std::unique_ptr<sql::Statement> stmt(con->createStatement());
        std::string query =
            "SELECT "
            "o.order_id, "
            "c.name_ AS client_name, "
            "CONCAT(d.first_name, ' ', d.last_name) AS driver_name, "
            "v.registration_number AS vehicle, "
            "td.trip_id AS tripdetails, "
            "tl.login_id AS triplog, "
            "o.route_from, o.route_to, o.order_status "
            "FROM `order` o "
            + type + " JOIN client c ON o.client_id = c.client_id "
            + type + " JOIN driver d ON o.driver_id = d.driver_id "
            + type + " JOIN vehicle v ON o.vehicle_id = v.vehicle_id "
            + type + " JOIN tripdetails td ON o.order_id = td.order_id "
            + type + " JOIN triplog tl ON o.order_id = tl.order_id "
            "LIMIT " + std::to_string(limit) + ";";

        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery(query));

        int rows = 0;
        while (res->next()) rows++;  

        auto end = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(end - start).count();

        std::cout << type << " JOIN | LIMIT " << limit << " | Time: " << ms
            << " ms (" << rows << " rows)\n";

        return ms;
    }
    catch (sql::SQLException& e) {
        std::cerr << "Query error: " << e.what() << std::endl;
        return 0.0;
    }
}

void averageJoinTime(const std::string& type, int limit, int repeats, sql::Connection* con) {
    std::vector<double> times;
    times.reserve(repeats);

    for (int i = 0; i < repeats; ++i) {
        times.push_back(testRawJoin(type, limit, con));
    }

    double sum = std::accumulate(times.begin(), times.end(), 0.0);
    double avg = sum / times.size();
    double min = *std::min_element(times.begin(), times.end());
    double max = *std::max_element(times.begin(), times.end());

    std::cout << "\nRESULTS for JOIN \"" << type << "\":\n";
    std::cout << "  Avg: " << avg << " ms\n";
    std::cout << "  Min: " << min << " ms\n";
    std::cout << "  Max: " << max << " ms\n\n";
}

//Insert

struct InsertResult {
    double time;  
    int lastInsertId; 
};

InsertResult testRawInsert(sql::Connection* con, const std::string& table) {
    InsertResult result{};
    try {
        auto start = std::chrono::high_resolution_clock::now();

        std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<int> dist(0, 99999);
        std::string suffix = std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + "_" + std::to_string(dist(rng));

        std::unique_ptr<sql::Statement> stmt(con->createStatement());
        std::stringstream query;

        if (table == "client") {
            query << "INSERT INTO client (client_type, name_, contact_person, phone, email) VALUES ("
                << "'TestType','Client_" << suffix << "','Person_" << suffix << "','9" << dist(rng)
                << "','test" << suffix << "@mail.com')";
        }
        else if (table == "driver") {
            query << "INSERT INTO driver (first_name,last_name,license_number,license_category,email,phone) VALUES ("
                << "'Driver_" << suffix << "','Last_" << suffix << "','LN" << suffix << "','B','driver" << suffix
                << "@mail.com','9" << dist(rng) << "')";
        }
        else if (table == "vehicle") {
            query << "INSERT INTO vehicle (registration_number, vehicle_type, make, model, technical_characteristics, status) VALUES ("
                << "'REG" << suffix << "','Truck','Make_" << suffix << "','Model_" << suffix << "','Specs','available')";
        }
        else if (table == "order") {
            std::ifstream f("orders.json");
            if (!f.is_open()) {
                std::cerr << "ERROR: cannot open orders.json\n";
                return result;
            }

            json orders;
            try {
                f >> orders;
            }
            catch (json::exception& e) {
                std::cerr << "JSON parse error: " << e.what() << std::endl;
                return result;
            }

            if (orders.empty()) {
                std::cerr << "orders.json is empty\n";
                return result;
            }

            std::mt19937 rng(std::random_device{}());
            std::uniform_int_distribution<int> distJson(0, orders.size() - 1);
            int idx = distJson(rng);

            json o = orders[idx];

            auto escapeSql = [](const std::string& s) {
                std::string r;
                for (char c : s) {
                    if (c == '\'') r += "\\'";
                    else r += c;
                }
                return r;
                };

            auto getDateOrDefault = [](const json& j, const std::string& key) {
                if (j.contains(key) && !j[key].is_null() && !j[key].get<std::string>().empty()) {
                    std::string dt = j[key].get<std::string>();
                    std::replace(dt.begin(), dt.end(), 'T', ' '); 
                    return dt;
                }
                return std::string("1970-01-01 00:00:00"); 
                };

            int client_id = o["client_id"].get<int>();
            int driver_id = o["driver_id"].get<int>();
            int vehicle_id = o["vehicle_id"].get<int>();
            std::string route_from = escapeSql(o["route_from"].is_null() ? "" : o["route_from"].get<std::string>());
            std::string route_to = escapeSql(o["route_to"].is_null() ? "" : o["route_to"].get<std::string>());
            std::string planned = getDateOrDefault(o, "planned_departure");
            std::string arrival = getDateOrDefault(o, "planned_arrival");
            std::string cargo = escapeSql(o["cargo_details"].is_null() ? "" : o["cargo_details"].get<std::string>());
            std::string status = escapeSql(o["order_status"].is_null() ? "" : o["order_status"].get<std::string>());

            query << "INSERT INTO `order` "
                << "(client_id, driver_id, vehicle_id, route_from, route_to, planned_departure_time, planned_arrival_time, cargo_details, order_status) "
                << "VALUES ("
                << client_id << ", "
                << driver_id << ", "
                << vehicle_id << ", '"
                << route_from << "', '"
                << route_to << "', '"
                << planned << "', '"
                << arrival << "', '"
                << cargo << "', '"
                << status << "')";
        }

        else if (table == "tripdetails") {
            std::unique_ptr<sql::ResultSet> orders(stmt->executeQuery("SELECT order_id FROM `order` LIMIT 10"));
            if (!orders->next()) { std::cerr << "No available order for tripdetails insert\n"; return result; }
            int orderId = orders->getInt("order_id");

            query << "INSERT INTO tripdetails (order_id, actual_trip_status, fuel_cost, other_expenses, total_cost, revenue) "
                << "VALUES (" << orderId << ",'completed',50.0,5.0,55.0,100.0)";
        }
        else if (table == "triplog") {
            std::unique_ptr<sql::ResultSet> trips(stmt->executeQuery(
                "SELECT t.trip_id, t.order_id FROM tripdetails t LIMIT 10"
            ));
            if (!trips->next()) {
                std::cerr << "No available trip for triplog insert\n";
                return result;
            }

            int tripId = trips->getInt("trip_id");
            int orderId = trips->getInt("order_id");

            query << "INSERT INTO triplog "
                << "(trip_id, order_id, actual_departure_time, actual_arrival_time, driver_comments) "
                << "VALUES ("
                << tripId << ", "
                << orderId << ", NOW(), NOW(), 'Test log " << suffix << "')";
        }
        else {
            std::cerr << "Invalid table: " << table << std::endl;
            return result;
        }

        stmt->execute(query.str());
        result.lastInsertId = 0;

        auto end = std::chrono::high_resolution_clock::now();
        result.time = std::chrono::duration<double, std::milli>(end - start).count();

        return result;
    }
    catch (sql::SQLException& e) {
        std::cerr << "INSERT error: " << e.what() << std::endl;
        return result;
    }
}

std::vector<int> averageInsertRaw(sql::Connection* con, const std::string& table, int repeats) {
    std::vector<double> times;
    std::vector<int> insertedIds;

    times.reserve(repeats);
    insertedIds.reserve(repeats);

    for (int i = 0; i < repeats; ++i) {
        InsertResult res = testRawInsert(con, table);
        times.push_back(res.time);
        insertedIds.push_back(res.lastInsertId);
    }

    double sum = std::accumulate(times.begin(), times.end(), 0.0);
    double avg = sum / times.size();

    double min = *std::min_element(times.begin(), times.end());
    double max = *std::max_element(times.begin(), times.end());

    std::cout << "\nRESULTS for INSERT \"" << table << "\"\n";
    std::cout << "  Avg: " << avg << " ms\n";
    std::cout << "  Min: " << min << " ms\n";
    std::cout << "  Max: " << max << " ms\n";

    return insertedIds;
}


//Update

double testRawUpdate(sql::Connection* con, const std::string& table, int id) {
    try {
        auto start = std::chrono::high_resolution_clock::now();
        std::unique_ptr<sql::Statement> stmt(con->createStatement());

        if (table == "client") {
            stmt->execute("UPDATE client SET phone = CONCAT('9', FLOOR(RAND()*1000000000)) WHERE client_id = " + std::to_string(id));
        }
        else if (table == "driver") {
            stmt->execute("UPDATE driver SET email = CONCAT('driver', FLOOR(RAND()*10000), '@mail.com') WHERE driver_id = " + std::to_string(id));
        }
        else if (table == "vehicle") {
            stmt->execute("UPDATE vehicle SET status = IF(status='available','busy','available') WHERE vehicle_id = " + std::to_string(id));
        }
        else if (table == "order") {
            stmt->execute("UPDATE `order` SET route_from='CityX', route_to='CityY' WHERE order_id = " + std::to_string(id));
        }
        else if (table == "tripdetails") {
            stmt->execute("UPDATE tripdetails SET actual_trip_status='delayed' WHERE trip_id = " + std::to_string(id));
        }
        else if (table == "triplog") {
            stmt->execute("UPDATE triplog SET driver_comments='Updated test log' WHERE login_id = " + std::to_string(id));
        }
        else {
            std::cerr << "UPDATE not implemented for table: " << table << std::endl;
        }

        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::milli>(end - start).count();
    }
    catch (sql::SQLException& e) {
        std::cerr << "UPDATE error: " << e.what() << std::endl;
        return 0.0;
    }
}

double averageUpdateTime(sql::Connection* con, const std::string& table, const std::vector<int>& ids, int repeats) {
    std::vector<double> times;
    times.reserve(repeats);

    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<size_t> dist(0, ids.size() - 1);

    for (int i = 0; i < repeats; ++i) {
        int id = ids[dist(rng)];
        double t = testRawUpdate(con, table, id);
        times.push_back(t);
    }

    double avg = std::accumulate(times.begin(), times.end(), 0.0) / times.size();
    double minTime = *std::min_element(times.begin(), times.end());
    double maxTime = *std::max_element(times.begin(), times.end());

    std::cout << "\nRESULTS for UPDATE \"" << table << "\"\n";
    std::cout << "  Avg: " << avg << " ms\n";
    std::cout << "  Min: " << minTime << " ms\n";
    std::cout << "  Max: " << maxTime << " ms\n";

    return avg;
}


//Delete

double testRawDelete(sql::Connection* con, const std::string& table, int id) {
    try {
        auto start = std::chrono::high_resolution_clock::now();
        std::unique_ptr<sql::Statement> stmt(con->createStatement());

        if (table == "client") {
            stmt->execute("DELETE FROM client WHERE client_id = " + std::to_string(id));
        }
        else if (table == "driver") {
            stmt->execute("DELETE FROM driver WHERE driver_id = " + std::to_string(id));
        }
        else if (table == "vehicle") {
            stmt->execute("DELETE FROM vehicle WHERE vehicle_id = " + std::to_string(id));
        }
        else if (table == "order") {
            stmt->execute("DELETE FROM `order` WHERE order_id = " + std::to_string(id));
        }
        else if (table == "tripdetails") {
            stmt->execute("DELETE FROM tripdetails WHERE trip_id = " + std::to_string(id));
        }
        else if (table == "triplog") {
            stmt->execute("DELETE FROM triplog WHERE login_id = " + std::to_string(id));
        }
        else {
            std::cerr << "DELETE not implemented for table: " << table << std::endl;
        }

        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::milli>(end - start).count();
    }
    catch (sql::SQLException& e) {
        std::cerr << "DELETE error: " << e.what() << std::endl;
        return 0.0;
    }
}

double averageDeleteTime(sql::Connection* con, const std::string& table, const std::vector<int>& ids) {
    std::vector<double> times;
    times.reserve(ids.size());

    for (auto id : ids) {
        double t = testRawDelete(con, table, id);
        times.push_back(t);
    }

    if (times.empty()) {
        std::cerr << "No DELETE operations were performed.\n";
        return 0.0;
    }

    double avg = std::accumulate(times.begin(), times.end(), 0.0) / times.size();
    double minTime = *std::min_element(times.begin(), times.end());
    double maxTime = *std::max_element(times.begin(), times.end());

    std::cout << "\nRESULTS for DELETE \"" << table << "\"\n";
    std::cout << "  Avg: " << avg << " ms\n";
    std::cout << "  Min: " << minTime << " ms\n";
    std::cout << "  Max: " << maxTime << " ms\n";

    return avg;
}

