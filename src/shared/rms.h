#ifndef SHARED_RMS_H_
#define SHARED_RMS_H_ 1

#include <exception>
#include <set>
#include <string>
#include <optional>
#include "address.h"

class UnknownServerAddr : public std::exception {
    private:
        Address server;
        std::string message;
    public:
        UnknownServerAddr(Address server_addr);
        Address server_addr() const;
        virtual char const *what() const noexcept;
};


class RmGroup : public Serializable, public Deserializable {
    private:
        std::set<Address> servers;
        std::optional<Address> primary;

    public:
        RmGroup();
        RmGroup(
            std::set<Address> const& servers,
            std::optional<Address> primary
        );

        std::set<Address> const& server_addrs() const;
        std::optional<Address> primary_addr() const;
        bool contains_server(Address server_addr) const;

        bool add_server(Address server_addr);
        bool remove_server(Address server_addr);
        void primary_elected(Address primary_addr);
        void primary_failed();

        virtual void serialize(Serializer& stream) const;
        virtual void deserialize(Deserializer& stream);
};

#endif
