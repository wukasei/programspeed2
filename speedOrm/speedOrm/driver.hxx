#pragma once
#include <string>
#include <odb/core.hxx>

// Map this class to the "driver" table
// #pragma db object table("driver")
#pragma db object pointer(std::shared_ptr) table("driver")
class Driver
{
public:
    Driver() = default;

    Driver(const std::string &first_name,
           const std::string &last_name,
           const std::string &license_number,
           const std::string &license_category,
           const std::string &email,
           const std::string &phone)
        : first_name_(first_name),
          last_name_(last_name),
          license_number_(license_number),
          license_category_(license_category),
          email_(email),
          phone_(phone)
    {
    }

// --- Fields ---
#pragma db id auto
    unsigned long driver_id_; // PRIMARY KEY (AUTO_INCREMENT)

    std::string first_name_;
    std::string last_name_;

#pragma db unique
    std::string license_number_; // UNIQUE

    std::string license_category_;
    std::string email_;
    std::string phone_;

private:
    friend class odb::access;
};
