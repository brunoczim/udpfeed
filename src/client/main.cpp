#include <cstdint>
#include <cstdlib>
#include <string>
#include <iostream>
#include "../shared/username.h"
#include "session_manager.h"
#include "interface.h"

struct Arguments {
    Username username;
    std::string server_address;
    uint16_t server_port;
};

void print_help(void);

Arguments parse_arguments(int argc, char const *argv[]);

int main(int argc, char const *argv[])
{
    Arguments arguments = parse_arguments(argc, argv);

    Socket udp(1024);
    ReliableSocket socket(std::move(udp));

    return 0;
}

void print_help(void)
{
    std::cerr
        << "Usage: ./app_client <username> <server-address> <server-port>"
        << std::endl;
}

Arguments parse_arguments(int argc, char const *argv[])
{
    Arguments arguments;
    if (argc != 4) {
        print_help();
        exit(1);
    }

    try {
        arguments.username = Username(argv[1]);
    } catch (InvalidUsername const& exception) {
        std::cerr << exception.what() << std::endl;
        exit(1);
    }

    arguments.server_address = argv[2];

    try {
        arguments.server_port = parse_udp_port(argv[3]);
    } catch (InvalidUdpPort const& exception) {
        std::cerr << exception.what() << std::endl;
        exit(1);
    }

    return arguments;
}
