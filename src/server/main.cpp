#include <cstdint>
#include <cstdlib>
#include <string>
#include <iostream>
#include "../shared/address.h"

struct Arguments {
    std::string bind_address;
    UdpPort bind_port;
};

void print_help(void);

Arguments parse_arguments(int argc, char const *argv[]);

int main(int argc, char const *argv[])
{
    Arguments arguments = parse_arguments(argc, argv);
    return 0;
}

void print_help(void)
{
    std::cerr
        << "Usage: ./app_server <bind-address> <bind-port>"
        << std::endl;
}

Arguments parse_arguments(int argc, char const *argv[])
{
    Arguments arguments;
    if (argc != 3) {
        print_help();
        exit(1);
    }
    arguments.bind_address = argv[1];
    try {
        arguments.bind_port = parse_udp_port(argv[2]);
    } catch (InvalidUdpPort const& exception) {
        std::cerr << exception.what() << std::endl;
        exit(1);
    }
    return arguments;
}
