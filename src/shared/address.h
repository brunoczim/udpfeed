#ifndef SHARED_ADDRESS_H_
#define SHARED_ADDRESS_H_ 1

#include <string>
#include <stdexcept>
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

#endif
