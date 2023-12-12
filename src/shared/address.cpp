#include <iterator>
#include <sstream>
#include <arpa/inet.h>
#include "address.h"

InvalidUdpPort::InvalidUdpPort(
    char const *port,
    std::string const &message
): message()
{
    this->message += "port '";
    this->message += port;
    this->message += "' is invalid: ";
    this->message += message;
}

const char *InvalidUdpPort::what() const noexcept
{
    return message.c_str();
}

uint16_t parse_udp_port(char const *content)
{
    uint16_t port = 0;
    try {
        size_t parse_end_pos = 0;
        int parsed_int = std::stoi(content, &parse_end_pos);
        if (parse_end_pos == 0) {
            throw InvalidUdpPort(content, "must contain a digit");
        }
        if (content[parse_end_pos] != 0) {
            throw InvalidUdpPort(content, "contains invalid digit");
        }
        if (parsed_int < UDP_PORT_MIN || parsed_int > UDP_PORT_MAX) {
            throw InvalidUdpPort(content, "number is out of range");
        }
        port = parsed_int;
    } catch (std::invalid_argument const& exception) {
        throw InvalidUdpPort(content, "invalid number given");
    } catch (std::out_of_range const& exception) {
        throw InvalidUdpPort(content, "number is out of range");
    }
    return port;
}

uint16_t parse_udp_port(std::string const& content)
{
    return parse_udp_port(content.c_str());
}

uint32_t make_ipv4(std::array<uint8_t, 4> bytes)
{
    uint32_t ipv4 = 0;
    for (auto it = bytes.begin(); it != bytes.end(); it++) {
        ipv4 <<= 8;
        ipv4 |= *it;
    }
    return ipv4;
}

Address::Address() : Address(0, 0)
{
}

Address::Address(uint32_t ipv4, uint16_t port) : ipv4(ipv4), port(port)
{
}

Address::Address(std::array<uint8_t, 4> ipv4, uint16_t port) :
    ipv4(make_ipv4(ipv4)),
    port(port)
{
}

bool Address::operator==(Address const& other) const
{
    return this->ipv4 == other.ipv4 && this->port == other.port;
}

bool Address::operator!=(Address const& other) const
{
    return this->ipv4 != other.ipv4 || this->port != other.port;
}

bool Address::operator<(Address const& other) const
{
    if (this->ipv4 < other.ipv4) {
        return true;
    }
    return this->ipv4 == other.ipv4 && this->port < other.port;
}

bool Address::operator<=(Address const& other) const
{
    return this->ipv4 <= other.ipv4 && this->port <= other.port;
}

bool Address::operator>(Address const& other) const
{
    if (this->ipv4 > other.ipv4) {
        return true;
    }
    return this->ipv4 == other.ipv4 && this->port > other.port;
}

bool Address::operator>=(Address const& other) const
{
    return this->ipv4 >= other.ipv4 && this->port >= other.port;
}

std::string Address::to_string() const
{
    uint32_t ipv4 = htonl(this->ipv4);
    uint16_t port = htons(this->port);
    std::stringstream sstream;

    for (size_t i = 0; i < 4; i++) {
        sstream << (ipv4 & 0xff);
        if (i < 3) {
            sstream << ".";
        } else {
            sstream << ":";
        }
        ipv4 >>= 8;
    }
    sstream << port;
    return sstream.str();
}
