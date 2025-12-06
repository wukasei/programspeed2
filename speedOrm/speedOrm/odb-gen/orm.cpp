#include <iostream>
#include <chrono>
#include <numeric>
#include <vector>
#include <memory>
#include <string>

#include <odb/mysql/database.hxx>
#include <odb/transaction.hxx>
#include <odb/result.hxx>

#include "client.hxx"
#include "client-odb.hxx"
#include "vehicle.hxx"
#include "vehicle-odb.hxx"
#include "driver.hxx"
#include "driver-odb.hxx"
#include "order.hxx"
#include "order-odb.hxx"
#include "triplog.hxx"
#include "triplog-odb.hxx"
#include "tripdetails.hxx"
#include "tripdetails-odb.hxx"

using namespace std;
using namespace std::chrono;

// --- Connect to MySQL ---
shared_ptr<odb::mysql::database> connectDB()
{
    auto db = make_shared<odb::mysql::database>(
        "speedorm", "123456",
        "transportcompany",
        "localhost");
    cout << "Connected to transport_company using ODB!\n";
    return db;
}

// --- Template function to measure SELECT time for any table ---
template <typename T>
double testOrmSelectTime(shared_ptr<odb::mysql::database> db, int limit)
{
    auto start = high_resolution_clock::now();

    typedef odb::result<T> Result;
    typedef odb::query<T> Query;

    odb::transaction t(db->begin());
    Result r(db->query<T>(Query::id > 0)); // assumes each table has id as primary key
    int rows = 0;
    for (const auto &obj : r)
    {
        if (++rows >= limit)
            break; // simulate JS limit
    }
    t.commit();

    auto end = high_resolution_clock::now();
    double ms = duration<double, milli>(end - start).count();

    cout << "SELECT " << typeid(T).name()
         << " | LIMIT " << limit
         << " | Time: " << ms << " ms (" << rows << " rows)\n";

    return ms;
}

// --- Average SELECT across multiple tables ---
void averageSelectAllTables(shared_ptr<odb::mysql::database> db,
                            int limit, int repeats)
{
    cout << "\n--- Average SELECT times ---\n";

    struct Result
    {
        string table;
        double avg;
    };
    vector<Result> results;

    // Client
    {
        vector<double> times;
        for (int i = 0; i < repeats; ++i)
            times.push_back(testOrmSelectTime<Client>(db, limit));
        double avg = accumulate(times.begin(), times.end(), 0.0) / times.size();
        results.push_back({"Client", avg});
    }

    // Vehicle
    {
        vector<double> times;
        for (int i = 0; i < repeats; ++i)
            times.push_back(testOrmSelectTime<Vehicle>(db, limit));
        double avg = accumulate(times.begin(), times.end(), 0.0) / times.size();
        results.push_back({"Vehicle", avg});
    }

    // Driver
    {
        vector<double> times;
        for (int i = 0; i < repeats; ++i)
            times.push_back(testOrmSelectTime<Driver>(db, limit));
        double avg = accumulate(times.begin(), times.end(), 0.0) / times.size();
        results.push_back({"Driver", avg});
    }

    // Order
    {
        vector<double> times;
        for (int i = 0; i < repeats; ++i)
            times.push_back(testOrmSelectTime<Order>(db, limit));
        double avg = accumulate(times.begin(), times.end(), 0.0) / times.size();
        results.push_back({"Order", avg});
    }

    // TripLog
    {
        vector<double> times;
        for (int i = 0; i < repeats; ++i)
            times.push_back(testOrmSelectTime<TripLog>(db, limit));
        double avg = accumulate(times.begin(), times.end(), 0.0) / times.size();
        results.push_back({"TripLog", avg});
    }

    // TripDetails
    {
        vector<double> times;
        for (int i = 0; i < repeats; ++i)
            times.push_back(testOrmSelectTime<TripDetails>(db, limit));
        double avg = accumulate(times.begin(), times.end(), 0.0) / times.size();
        results.push_back({"TripDetails", avg});
    }

    cout << "\n--- Summary SELECT ---\n";
    for (const auto &r : results)
    {
        cout << r.table << string(12 - r.table.size(), ' ')
             << "| Average: " << r.avg << " ms\n";
    }
}
