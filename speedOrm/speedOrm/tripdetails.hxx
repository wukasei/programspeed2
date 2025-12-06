#pragma once
#include <string>
#include <memory>
#include <odb/core.hxx>
#include <odb/lazy-ptr.hxx>
#include "order.hxx"

class Order;

// Map this class to the "tripdetails" table
// #pragma db object table("tripdetails")
#pragma db object pointer(std::shared_ptr) table("tripdetails")
class TripDetails
{
public:
    TripDetails() = default;

    TripDetails(std::shared_ptr<Order> order,
                const std::string &actual_trip_status,
                double fuel_cost,
                double other_expenses,
                double total_cost,
                double revenue)
        : order_(order),
          actual_trip_status_(actual_trip_status),
          fuel_cost_(fuel_cost),
          other_expenses_(other_expenses),
          total_cost_(total_cost),
          revenue_(revenue)
    {
    }

// PRIMARY KEY (AUTO_INCREMENT)
#pragma db id auto
    unsigned long long trip_id_;

    // Foreign key to Order
    // #pragma db not_null column("order_id")
    //    std::shared_ptr<Order> order_;

#pragma db not_null column("order_id")
    odb::lazy_shared_ptr<Order> order_;

    std::string actual_trip_status_;

    double fuel_cost_;
    double other_expenses_;
    double total_cost_;
    double revenue_;

private:
    friend class odb::access;
};
