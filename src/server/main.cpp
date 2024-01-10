#include <cstdint>
#include <cstdlib>
#include <string>
#include <iostream>
#include "data.h"
#include "comm_manager.h"
#include "prof_manager.h"
#include "notif_manager.h"
#include "../shared/shutdown.h"
#include "../shared/log.h"

struct Arguments {
    Address bind_address;
};

void print_help(void);

Arguments parse_arguments(int argc, char const *argv[]);

int main(int argc, char const *argv[])
{
    using namespace std::chrono_literals;

    Arguments arguments = parse_arguments(argc, argv);

    std::ios::sync_with_stdio();

    Logger::set_output(&std::cerr);

    ThreadTracker thread_tracker;

    Logger::with([bind_address = arguments.bind_address] (auto& output) {
        output << "Binding to " << bind_address.to_string() << std::endl;
    });

    Socket udp(arguments.bind_address, 1024);
    std::shared_ptr<ReliableSocket> socket(new ReliableSocket(std::move(udp)));

    Logger::with([&socket] (auto& output) {
        socket->config().report(output);
    });

    std::shared_ptr<ServerProfileTable> profile_table(new ServerProfileTable);

    profile_table->load();

    Channel<ReliableSocket::ReceivedReq> comm_to_prof_man;
    Channel<Enveloped> notif_to_comm_man;
    Channel<Username> prof_to_notif_man;

    Channel<ReliableSocket::ReceivedReq>::Receiver prof_receiver =
        comm_to_prof_man.receiver;
    Channel<Enveloped>::Receiver comm_receiver =
        notif_to_comm_man.receiver;
    Channel<Username>::Receiver notif_receiver =
        prof_to_notif_man.receiver;

    Logger::with([] (auto& output) {
        output << "Starting communication manager" << std::endl;
    });

    start_server_communication_manager(
        thread_tracker,
        socket,
        std::move(comm_to_prof_man.sender),
        std::move(notif_to_comm_man.receiver)
    );

    Logger::with([] (auto& output) {
        output << "Starting profile manager" << std::endl;
    });

    start_server_profile_manager(
        thread_tracker,
        profile_table,
        std::move(prof_to_notif_man.sender),
        std::move(comm_to_prof_man.receiver)
    );

    Logger::with([] (auto& output) {
        output << "Starting notification manager" << std::endl;
    });

    start_server_notification_manager(
        thread_tracker,
        profile_table,
        std::move(notif_to_comm_man.sender),
        std::move(prof_to_notif_man.receiver)
    );

    Logger::with([] (auto& output) {
        output << "Press Ctrl-C or Ctrl-D to shutdown" << std::endl;
    });

    wait_for_graceful_shutdown(SHUTDOWN_ACTIVE_EOF);

    Logger::with([] (auto& output) {
        output << std::endl << "Shutting down..." << std::endl;
    });

    prof_receiver.disconnect();
    comm_receiver.disconnect();
    notif_receiver.disconnect();

    socket->disconnect_timeout(50 * 1000 * 1000, 10);

    thread_tracker.join_all();

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

    try {
        arguments.bind_address.ipv4 = parse_ipv4(argv[1]);
    } catch (InvalidIpv4 const& exception) {
        std::cerr << exception.what() << std::endl;
        exit(1);
    }

    try {
        arguments.bind_address.port = parse_udp_port(argv[2]);
    } catch (InvalidUdpPort const& exception) {
        std::cerr << exception.what() << std::endl;
        exit(1);
    }

    return arguments;
}
