#include <iostream>
#include <chrono>
#include <numeric>
#include <vector>
#include <memory>
#include <string>
#include <iomanip>
#include <fstream>
#include "json.hpp"
using json = nlohmann::json;

#include <odb/mysql/database.hxx>
#include <odb/transaction.hxx>
#include <odb/result.hxx>
#include <odb/session.hxx>

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

template <typename T>
struct TableTraits;

template <typename T>
struct TableTraits
{
    using QueryType = void;

    static auto pk()
    {
        return typename TableTraits<T>::QueryType();
    }

    static void loadRelations(T &) {}
};

template <>
struct TableTraits<Client>
{
    using QueryType = odb::query<Client>;

    static QueryType pk()
    {
        return QueryType::client_id > 0;
    }

    static void loadRelations(Client &c)
    {
    }
};

template <>
struct TableTraits<Vehicle>
{
    using QueryType = odb::query<Vehicle>;

    static QueryType pk()
    {
        return QueryType::vehicle_id > 0;
    }

    static void loadRelations(Vehicle &v)
    {
    }
};

template <>
struct TableTraits<Driver>
{
    using QueryType = odb::query<Driver>;

    static QueryType pk()
    {
        return QueryType::driver_id > 0;
    }

    static void loadRelations(Driver &d)
    {
       
    }
};

template <>
struct TableTraits<Order>
{
    using QueryType = odb::query<Order>;

    static QueryType pk()
    {
        return QueryType::order_id > 0;
    }

    static void loadRelations(Order &o)
    {
        if (o.client_)
            o.client_.load();
        if (o.driver_)
            o.driver_.load();
        if (o.vehicle_)
            o.vehicle_.load();

        // Order → TripLog (load only child; do NOT load parent Order)
        for (auto &x : o.triplogs_)
            x.load();

        // Order → TripDetail
        for (auto &x : o.tripdetails_)
            x.load();
    }
};

template <>
struct TableTraits<TripLog>
{
    using QueryType = odb::query<TripLog>;

    static QueryType pk()
    {
        return QueryType::login_id > 0;
    }

    static void loadRelations(TripLog &t)
    {
        // TripLog → Order
        if (t.order_)
            t.order_.load();
        // DO NOT load Order relations here → recursion
    }
};

template <>
struct TableTraits<TripDetails>
{
    using QueryType = odb::query<TripDetails>;

    static QueryType pk()
    {
        return QueryType::trip_id > 0;
    }

    static void loadRelations(TripDetails &t)
    {
        // TripLog → Order
        if (t.order_)
            t.order_.load();
        // DO NOT load Order relations here → recursion
    }
};

std::shared_ptr<odb::mysql::database> connectDB()
{
    static std::shared_ptr<odb::mysql::database> db =
        std::make_shared<odb::mysql::database>(
            "speedorm", "123456",
            "transport_company",
            "localhost");

    return db; 
}

template <typename T>
double testOrmSelectTime(shared_ptr<odb::mysql::database> db, int limit)
{
    auto start = chrono::high_resolution_clock::now();

    using Query = typename TableTraits<T>::QueryType;
    odb::transaction t(db->begin());

    string limitClause = "LIMIT " + to_string(limit);
    odb::result<T> r(db->query<T>(TableTraits<T>::pk() + Query(limitClause)));

    int rows = 0;
    for (T &obj : r)
    {
        TableTraits<T>::loadRelations(obj);
        rows++;
    }

    t.commit();

    auto end = chrono::high_resolution_clock::now();
    double ms = duration<double, milli>(end - start).count();

    cout << "SELECT " << typeid(T).name()
         << " | LIMIT " << limit
         << " | Time: " << ms << " ms (" << rows << " rows)\n";

    return ms;
}


void averageSelectAllModels(shared_ptr<odb::mysql::database> db,
                            int limit, int repeats)
{
    struct R
    {
        string name;
        double avg;
        double min;
        double max;
    };
    vector<R> results;

    auto measure = [&](auto x, string name)
    {
        using T = decltype(x);

        vector<double> times;
        times.reserve(repeats);

        for (int i = 0; i < repeats; ++i)
            times.push_back(testOrmSelectTime<T>(db, limit));

        double sum = accumulate(times.begin(), times.end(), 0.0);
        double avg = sum / times.size();

        double minv = *min_element(times.begin(), times.end());
        double maxv = *max_element(times.begin(), times.end());

        results.push_back({name, avg, minv, maxv});
    };

    measure(Client{}, "Client");
    measure(Vehicle{}, "Vehicle");
    measure(Driver{}, "Driver");
    measure(Order{}, "Order");
    measure(TripLog{}, "TripLog");
    measure(TripDetails{}, "TripDetails");

    cout << "\n--- Summary SELECT Times ---\n";
    cout << setw(12) << left << "Model"
         << "| " << setw(10) << "AVG (ms)"
         << " | " << setw(10) << "MIN (ms)"
         << " | " << setw(10) << "MAX (ms)" << "\n";

    for (auto &r : results)
    {
        cout << setw(12) << left << r.name
             << "| " << setw(10) << r.avg
             << " | " << setw(10) << r.min
             << " | " << setw(10) << r.max
             << "\n";
    }
}

Order makeOrderFromJson(const json &j)
{
    Order o;

    if (j.contains("order_id"))
        o.order_id_ = j["order_id"].get<unsigned long>();

    o.route_from_ = j.value("route_from", "");
    o.route_to_ = j.value("route_to", "");
    o.cargo_details_ = j.value("cargo_details", "");
    o.order_status_ = j.value("order_status", "");
    o.planned_departure_time_ = j.value("planned_departure_time", "");
    o.planned_arrival_time_ = j.value("planned_arrival_time", "");
    o.client_id_ = j.value("client_id", 0UL);
    o.driver_id_ = j.value("driver_id", 0UL);
    o.vehicle_id_ = j.value("vehicle_id", 0UL);

    return o;
}

void benchmarkInsertOrders(shared_ptr<odb::mysql::database> db, int repeats)
{
    json rows;
    {
        ifstream f("orders.json");
        if (!f.is_open())
        {
            cerr << "ERROR: cannot open orders.json\n";
            return;
        }
        f >> rows;
    }

    vector<double> rowTimes; 
    rowTimes.reserve(rows.size() * repeats);

    for (int r = 0; r < repeats; ++r)
    {
        for (const auto &item : rows)
        {
            Order o = makeOrderFromJson(item);

            auto start = chrono::high_resolution_clock::now();

            {
                odb::transaction t(db->begin());
                db->persist(o);
                t.commit();
            }

            auto end = chrono::high_resolution_clock::now();
            double ms = duration<double, milli>(end - start).count(); 
            rowTimes.push_back(ms);
        }
    }

    double sum = accumulate(rowTimes.begin(), rowTimes.end(), 0.0);
    double avg = sum / rowTimes.size();
    double minv = *min_element(rowTimes.begin(), rowTimes.end());
    double maxv = *max_element(rowTimes.begin(), rowTimes.end());

    cout << "\n=== INSERT Orders Per-Row Benchmark (ms) ===\n";
    cout << "Rows tested:       " << rowTimes.size() << "\n";
    cout << "Repeats:           " << repeats << "\n\n";

    cout << "AVG per row:       " << avg  << " ms\n";
    cout << "MIN per row:       " << minv << " ms\n";
    cout << "MAX per row:       " << maxv << " ms\n\n";
}

void benchmarkInsertOrders_(shared_ptr<odb::mysql::database> db, int repeats)
{

    json rows;
    {
        std::ifstream f("orders.json");
        if (!f.is_open())
        {
            std::cerr << "ERROR: cannot open orders.json\n";
            return;
        }
        f >> rows;
    }

    vector<double> results;
    results.reserve(repeats);

    for (int r = 0; r < repeats; ++r)
    {
        auto start = chrono::high_resolution_clock::now();

        for (const auto &item : rows)
        {
            odb::transaction t(db->begin());

            Order o = makeOrderFromJson(item);
            db->persist(o);

            t.commit();
        }

        auto end = chrono::high_resolution_clock::now();
        double ms = duration<double, milli>(end - start).count();
        results.push_back(ms);
    }

    double sum = accumulate(results.begin(), results.end(), 0.0);
    double avg = sum / results.size();
    double minv = *min_element(results.begin(), results.end());
    double maxv = *max_element(results.begin(), results.end());

    cout << "\n=== INSERT Orders Benchmark ===\n";
    cout << "Rows in JSON: " << rows.size() << "\n";
    cout << "Repeats:      " << repeats << "\n";
    cout << "AVG:          " << avg << " ms\n";
    cout << "MIN:          " << minv << " ms\n";
    cout << "MAX:          " << maxv << " ms\n";
}


void benchmarkUpdateOrders(shared_ptr<odb::mysql::database> db, int repeats)
{
    json rows;
    {
        ifstream f("orders.json");
        if (!f.is_open())
        {
            cerr << "ERROR: cannot open orders.json\n";
            return;
        }
        f >> rows;
    }

    vector<double> rowTimes; 
    rowTimes.reserve(rows.size() * repeats);

    for (int r = 0; r < repeats; ++r)
    {
        for (const auto &item : rows)
        {
            unsigned long id = item.value("order_id", 0UL);
            if (id == 0)
                continue;

            auto start = chrono::high_resolution_clock::now();

            try
            {
                odb::transaction t(db->begin());

                shared_ptr<Order> o = db->load<Order>(id);

                o->route_from_              = item.value("route_from", "");
                o->route_to_                = item.value("route_to", "");
                o->cargo_details_           = item.value("cargo_details", "");
                o->order_status_            = item.value("order_status", "");
                o->planned_departure_time_  = item.value("planned_departure_time", "");
                o->planned_arrival_time_    = item.value("planned_arrival_time", "");
                o->client_id_               = item.value("client_id", 0UL);
                o->driver_id_               = item.value("driver_id", 0UL);
                o->vehicle_id_              = item.value("vehicle_id", 0UL);

                db->update(*o);

                t.commit();
            }
            catch (const odb::object_not_persistent &)
            {

            }

            auto end = chrono::high_resolution_clock::now();
            double ms = duration<double, milli>(end - start).count(); 
            rowTimes.push_back(ms);
        }
    }

    double sum  = accumulate(rowTimes.begin(), rowTimes.end(), 0.0);
    double avg  = sum / rowTimes.size();
    double minv = *min_element(rowTimes.begin(), rowTimes.end());
    double maxv = *max_element(rowTimes.begin(), rowTimes.end());

    cout << "\n=== UPDATE Orders Per-Row Benchmark (ms) ===\n";
    cout << "Rows tested:       " << rowTimes.size() << "\n";
    cout << "Repeats:           " << repeats << "\n\n";
    cout << "AVG per row:       " << avg  << " ms\n";
    cout << "MIN per row:       " << minv << " ms\n";
    cout << "MAX per row:       " << maxv << " ms\n\n";
}

void benchmarkDeleteOrders(shared_ptr<odb::mysql::database> db, int repeats)
{

    json rows;
    {
        ifstream f("orders.json");
        if (!f.is_open())
        {
            cerr << "ERROR: cannot open orders.json\n";
            return;
        }
        f >> rows;
    }

    vector<double> rowTimes;  
    rowTimes.reserve(rows.size() * repeats);

    for (int r = 0; r < repeats; ++r)
    {
        for (const auto &item : rows)
        {
            unsigned long id = item.value("order_id", 0UL);
            if (!id)
                continue;

            auto start = chrono::high_resolution_clock::now();

            try
            {
                odb::transaction t(db->begin());

                db->erase<Order>(id);

                t.commit();
            }
            catch (...)
            {

            }

            auto end = chrono::high_resolution_clock::now();
            double ms = duration<double, milli>(end - start).count();
            rowTimes.push_back(ms);
        }
    }

    double sum  = accumulate(rowTimes.begin(), rowTimes.end(), 0.0);
    double avg  = sum / rowTimes.size();
    double minv = *min_element(rowTimes.begin(), rowTimes.end());
    double maxv = *max_element(rowTimes.begin(), rowTimes.end());

    cout << "\n=== DELETE Orders Per-Row Benchmark (ms) ===\n";
    cout << "Rows tested:       " << rowTimes.size() << "\n";
    cout << "Repeats:           " << repeats << "\n\n";
    cout << "AVG per row:       " << avg  << " ms\n";
    cout << "MIN per row:       " << minv << " ms\n";
    cout << "MAX per row:       " << maxv << " ms\n\n";
}
