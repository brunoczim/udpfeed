#include <cctype>
#include <iterator>
#include <sstream>
#include <arpa/inet.h>
#include "address.h"

static void skip_whitespace(char const *string, size_t& pos);

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

InvalidIpv4::InvalidIpv4(
    char const *address,
    std::string const &message
): message()
{
    this->message += "ipv4 address '";
    this->message += address;
    this->message += "' is invalid: ";
    this->message += message;
}

const char *InvalidIpv4::what() const noexcept
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
        skip_whitespace(content, parse_end_pos);
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

uint32_t parse_ipv4(char const *content)
{
    std::array<uint8_t, 4> bytes;
    
    size_t pos = 0;
    size_t byte_i = 0;

    while (content[pos] != 0 && byte_i < 4) {
        try {
            size_t end_pos = 0;
            int parsed_int = std::stoi(content + pos, &end_pos);
            pos += end_pos;
            if (parsed_int < 0 || parsed_int > UINT8_MAX) {
                throw InvalidIpv4(content, "number is out of range");
            }
            bytes[byte_i] = parsed_int;
            byte_i++;
            skip_whitespace(content, pos);
            if (byte_i < 4 ) {
                if (content[pos] != '.') {
                    throw InvalidIpv4(content, "contains bad characters");
                } 
                pos++;
                skip_whitespace(content, pos);
            } else if (content[pos] != 0) {
                throw InvalidIpv4(content, "contains bad characters");
            }
        } catch (std::invalid_argument const& exception) {
            throw InvalidIpv4(content, "invalid number given");
        } catch (std::out_of_range const& exception) {
            throw InvalidIpv4(content, "number is out of range");
        }
    }

    if (byte_i != 4) {
        throw InvalidIpv4(content, "missing bytes");
    }

    return make_ipv4(bytes);
}

uint32_t parse_ipv4(std::string const& content)
{
    return parse_ipv4(content.c_str());
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

std::string ipv4_to_string(uint32_t ipv4)
{
    std::stringstream sstream;

    for (size_t i = 0; i < 4; i++) {
        sstream << (ipv4 >> 24);
        if (i < 3) {
            sstream << ".";
        }
        ipv4 <<= 8;
    }

    return sstream.str();
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

void Address::serialize(Serializer& stream) const
{
    stream << this->ipv4 << this->port;
}

void Address::deserialize(Deserializer& stream)
{
    stream >> this->ipv4 >> this->port;
}

std::string Address::to_string() const
{
    return ipv4_to_string(this->ipv4) + ":" + std::to_string(this->port);
}

static void skip_whitespace(char const *string, size_t& pos)
{
    while (string[pos] != 0 && std::isspace(string[pos])) {
        pos++;
    }
}
