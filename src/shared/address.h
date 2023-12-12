#ifndef SHARED_ADDRESS_H_
#define SHARED_ADDRESS_H_ 1

#include <string>
#include <stdexcept>
#include <array>
#include <cstdint>

#define UDP_PORT_MIN 1
#define UDP_PORT_MAX UINT16_MAX

class InvalidUdpPort : public std::exception {
    private:
        std::string message;
    public:
        InvalidUdpPort(char const *port, std::string const& message);

        virtual const char *what() const noexcept;
};

uint16_t parse_udp_port(char const *content);

uint16_t parse_udp_port(std::string const& content);

uint32_t make_ipv4(std::array<uint8_t, 4> bytes);

class Address {
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

        std::string to_string() const;
};

#endif
