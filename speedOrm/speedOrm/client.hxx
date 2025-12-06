#pragma once
#include <string>
#include <odb/core.hxx>

// Tell ODB this class maps to the "client" table
//#pragma db object table("client")
#pragma db object pointer(std::shared_ptr) table("client")
class Client
{
public:
    Client() = default;

    Client(const std::string& type,
           const std::string& name,
           const std::string& contact,
           const std::string& phone,
           const std::string& email)
        : client_type(type),
          name_(name),
          contact_person(contact),
          phone(phone),
          email(email)
    {}

    // Primary key
    #pragma db id auto
    unsigned long client_id;

    std::string client_type;
    std::string name_;
    std::string contact_person;
    std::string phone;
    std::string email;
};
