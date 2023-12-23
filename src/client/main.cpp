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

    ThreadTracker thread_tracker;
    Socket udp(1024);
    ReliableSocket socket(std::move(udp));

    Channel<std::shared_ptr<ClientInputCommand>> interface_to_session_man;
    Channel<std::shared_ptr<ClientOutputNotice>> session_man_to_interface; 

    Channel<std::shared_ptr<ClientInputCommand>>::Receiver
        session_main_receiver = interface_to_session_man.receiver;
    Channel<std::shared_ptr<ClientOutputNotice>>::Receiver
        interface_receiver = session_man_to_interface.receiver; 


    start_client_interface(
        thread_tracker,
        std::move(interface_to_session_man),
        std::move(session_man_to_interface)
    );

    start_client_session_manager(
        thread_tracker,
        arguments.username,
    );


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
