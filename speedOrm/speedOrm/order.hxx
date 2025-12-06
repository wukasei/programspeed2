#pragma once

#include <string>
#include <vector>
#include <memory>
#include <odb/core.hxx>
#include <odb/lazy-ptr.hxx>

#include "client.hxx"
#include "driver.hxx"
#include "vehicle.hxx"

#ifdef ODB_COMPILER
#include "triplog.hxx"
#include "tripdetails.hxx"
#endif

class TripLog;
class TripDetails;

#pragma db object pointer(std::shared_ptr) table("order")
class Order
{
public:
    Order() = default;

    // Constructor using only IDs (optional)
    Order(unsigned long client_id,
          const std::string& route_from,
          const std::string& route_to,
          const std::string& cargo_details,
          const std::string& order_status,
          unsigned long driver_id,
          unsigned long vehicle_id)
        : client_id_(client_id),
          driver_id_(driver_id),
          vehicle_id_(vehicle_id),
          route_from_(route_from),
          route_to_(route_to),
          cargo_details_(cargo_details),
          order_status_(order_status)
    {}

#pragma db id auto
    unsigned long order_id_;

    // ---------------------------------------------
    // REAL FOREIGN KEY COLUMNS (stored in database)
    // ---------------------------------------------
#pragma db not_null column("client_id")
    unsigned long client_id_;

#pragma db not_null column("driver_id")
    unsigned long driver_id_;

#pragma db not_null column("vehicle_id")
    unsigned long vehicle_id_;

    // ---------------------------------------------------
    // LAZY RELATIONS (NOT stored, only used for reading)
    // ---------------------------------------------------
#pragma db transient
    odb::lazy_shared_ptr<Client> client_;

#pragma db transient
    odb::lazy_shared_ptr<Driver> driver_;

#pragma db transient
    odb::lazy_shared_ptr<Vehicle> vehicle_;

    // Normal fields
    std::string route_from_;
    std::string route_to_;
    std::string cargo_details_;
    std::string order_status_;
    std::string planned_departure_time_;
    std::string planned_arrival_time_;

    // Inverse relations
#pragma db inverse(order_)
    std::vector<odb::lazy_shared_ptr<TripLog>> triplogs_;

#pragma db inverse(order_)
    std::vector<odb::lazy_shared_ptr<TripDetails>> tripdetails_;

public:

/*     // Load navigation relations after reading from DB
    void loadNavigation(odb::database& db)
    {
        client_  = db.load<Client>(client_id_);
        driver_  = db.load<Driver>(driver_id_);
        vehicle_ = db.load<Vehicle>(vehicle_id_);
    } */

private:
    friend class odb::access;
};
