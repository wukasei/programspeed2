#pragma once
#include <string>
#include <memory>
#include <odb/core.hxx>

#pragma db object table("triplog")

class triplogWR
{
public:
    triplogWR() = default;

    triplogWR(int trip_id,
            //std::shared_ptr<Order> order,
            
            const std::string& departure,
            const std::string& arrival,
            const std::string& comments)
        : trip_id_(trip_id),
          //order_(order),
          actual_departure_time_(departure),
          actual_arrival_time_(arrival),
          driver_comments_(comments)
    {}

#pragma db id auto
    unsigned long login_id_;

    int trip_id_;

#pragma db not_null column("order_id")
    //std::shared_ptr<Order> order_;

    // DO NOT USE DATETIME HERE
    std::string actual_departure_time_;

    // DO NOT USE DATETIME HERE
    std::string actual_arrival_time_;

    std::string driver_comments_;

private:
    friend class odb::access;
};
