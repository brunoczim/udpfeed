#include <cstdint>
#include <cstdlib>
#include <string>
#include <iostream>
#include "../shared/username.h"
#include "../shared/shutdown.h"
#include "../shared/log.h"
#include "session_manager.h"
#include "interface.h"

struct Arguments {
    Username username;
    Address server_address;
};

void print_help(void);

Arguments parse_arguments(int argc, char const *argv[]);

int main(int argc, char const *argv[])
{
    Arguments arguments = parse_arguments(argc, argv);

    std::ios::sync_with_stdio();

    Logger::set_output(&std::cerr);

    Logger::with([&arguments] (auto& output) {
        output
            << "Connecting to "
            << arguments.server_address.to_string()
            << " with username "
            << arguments.username.content()
            << std::endl
            << "Press Ctrl-C or Ctrl-D to disconnect"
            << std::endl;
    });

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
        std::move(interface_to_session_man.sender),
        std::move(session_man_to_interface.receiver)
    );

    start_client_session_manager(
        thread_tracker,
        arguments.username,
        arguments.server_address,
        std::move(socket),
        std::move(session_man_to_interface.sender),
        std::move(interface_to_session_man.receiver)
    );

    wait_for_graceful_shutdown(SHUTDOWN_PASSIVE_EOF);

    session_main_receiver.disconnect();
    interface_receiver.disconnect();

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

    try {
        arguments.server_address.ipv4 = parse_ipv4(argv[2]);
    } catch (InvalidIpv4 const& exception) {
        std::cerr << exception.what() << std::endl;
        exit(1);
    }

    try {
        arguments.server_address.port = parse_udp_port(argv[3]);
    } catch (InvalidUdpPort const& exception) {
        std::cerr << exception.what() << std::endl;
        exit(1);
    }

    return arguments;
}
