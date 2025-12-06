#pragma once
#include <string>
#include <odb/core.hxx>

// Map this class to the "vehicle" table
// #pragma db object table("vehicle")
#pragma db object pointer(std::shared_ptr) table("vehicle")
class Vehicle
{
public:
    Vehicle() = default;

    Vehicle(const std::string &registration_number,
            const std::string &vehicle_type,
            const std::string &make,
            const std::string &model,
            const std::string &technical_characteristics,
            const std::string &status)
        : registration_number_(registration_number),
          vehicle_type_(vehicle_type),
          make_(make),
          model_(model),
          technical_characteristics_(technical_characteristics),
          status_(status)
    {
    }

// --- Fields ---
#pragma db id auto
    unsigned long vehicle_id_; // PRIMARY KEY (AUTO_INCREMENT)

#pragma db unique
    std::string registration_number_; // UNIQUE

    std::string vehicle_type_;
    std::string make_;
    std::string model_;
    std::string technical_characteristics_;
    std::string status_;

private:
    friend class odb::access;
};
