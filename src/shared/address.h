#ifndef SHARED_ADDRESS_H_
#define SHARED_ADDRESS_H_ 1

#include <string>
#include <stdexcept>
#include <array>
#include <cstdint>

#include "serialization.h"

#define UDP_PORT_MIN 1
#define UDP_PORT_MAX UINT16_MAX

class InvalidAddress : public std::exception {
    private:
        std::string message;
    public:
        InvalidAddress(std::string const& message);

        virtual const char *what() const noexcept;
};

class InvalidUdpPort : public InvalidAddress {
    public:
        InvalidUdpPort(char const *port, std::string const& message);
};

class InvalidIpv4 : public InvalidAddress {
    public:
        InvalidIpv4(char const *ipv4, std::string const& message);
};

uint16_t parse_udp_port(char const *content);

uint16_t parse_udp_port(std::string const& content);

uint32_t parse_ipv4(char const *ipv4);

uint32_t parse_ipv4(std::string const& content);

uint32_t make_ipv4(std::array<uint8_t, 4> bytes);

std::string ipv4_to_string(uint32_t ipv4);

class Address : public Serializable, public Deserializable {
    public:
        uint32_t ipv4;
        uint16_t port;

        Address();
        Address(uint32_t ipv4, uint16_t port);
        Address(std::array<uint8_t, 4> ipv4, uint16_t port);

        bool operator==(Address const& other) const;
        bool operator!=(Address const& other) const;

        bool operator<(Address const& other) const;
        bool operator<=(Address const& other) const;
        bool operator>(Address const& other) const;
        bool operator>=(Address const& other) const;

        virtual void serialize(Serializer& stream) const;
        virtual void deserialize(Deserializer& stream);

        std::string to_string() const;

        static Address parse(char const *content);
        static Address parse(std::string const& content);
};

#endif
