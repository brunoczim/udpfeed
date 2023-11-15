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
    int parsed_int;
    size_t parse_end_pos;
    uint16_t port = 0;
    try {
        parsed_int = std::stoi(content, &parse_end_pos);
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
