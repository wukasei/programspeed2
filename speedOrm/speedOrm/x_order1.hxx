#pragma once
#include <string>
#include <memory>
#include <ctime>
#include <vector>
#include <odb/core.hxx>

#include "client.hxx"
#include "driver.hxx"
#include "vehicle.hxx"
//#include "triplog.hxx"  

class TripLog; // forward declaration

#pragma db object table("order1")
class Order1
{
public:
    Order1() = default;

    Order1(std::shared_ptr<Client> client,
          const std::string &route_from,
          const std::string &route_to,
          const std::string &cargo_details,
          const std::string &order_status,
          std::shared_ptr<Driver> driver,
          std::shared_ptr<Vehicle> vehicle)
        : client_(client),
          route_from_(route_from),
          route_to_(route_to),
          cargo_details_(cargo_details),
          order_status_(order_status),
          driver_(driver),
          vehicle_(vehicle)
    {
    }

#pragma db id auto
    unsigned long order_id_;

#pragma db not_null column("client_id")
    std::shared_ptr<Client> client_;

#pragma db not_null column("driver_id")
    std::shared_ptr<Driver> driver_;

#pragma db not_null column("vehicle_id")
    std::shared_ptr<Vehicle> vehicle_;

    std::string route_from_;
    std::string route_to_;
    std::string cargo_details_;
    std::string order_status_;

// work
//#pragma db inverse(order_)
//    std::vector<std::shared_ptr<TripLog>> triplogs_;

#pragma db inverse(order_)
std::vector<std::shared_ptr<TripLog>> triplogs_; 

private:
    friend class odb::access;
};
