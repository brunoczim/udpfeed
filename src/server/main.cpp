#include <cstdint>
#include <cstdlib>
#include <string>
#include <iostream>
#include "data.h"
#include "comm_manager.h"
#include "prof_manager.h"
#include "notif_manager.h"
#include "../shared/shutdown.h"

struct Arguments {
    std::string bind_address;
    uint16_t bind_port;
};

void print_help(void);

Arguments parse_arguments(int argc, char const *argv[]);

int main(int argc, char const *argv[])
{
    Arguments arguments = parse_arguments(argc, argv);

    ThreadTracker thread_tracker;

    Socket udp(1024, arguments.bind_port);
    ReliableSocket socket(std::move(udp));

    std::shared_ptr<ServerProfileTable> profile_table(new ServerProfileTable);

    Channel<ReliableSocket::ReceivedReq> comm_to_prof_man;
    Channel<Enveloped> notif_to_comm_man;
    Channel<Username> prof_to_notif_man;

    Channel<ReliableSocket::ReceivedReq>::Receiver prof_receiver =
        comm_to_prof_man.receiver;
    Channel<Enveloped>::Receiver comm_receiver =
        notif_to_comm_man.receiver;
    Channel<Username>::Receiver notif_receiver =
        prof_to_notif_man.receiver;


    start_server_communication_manager(
        thread_tracker,
        std::move(socket),
        std::move(comm_to_prof_man.sender),
        std::move(notif_to_comm_man.receiver)
    );

    start_server_profile_manager(
        thread_tracker,
        profile_table,
        std::move(prof_to_notif_man.sender),
        std::move(comm_to_prof_man.receiver)
    );

    start_server_notification_manager(
        thread_tracker,
        profile_table,
        std::move(notif_to_comm_man.sender),
        std::move(prof_to_notif_man.receiver)
    );

    GracefulShutdown::set_action([
        prof_receiver = std::move(prof_receiver),
        comm_receiver = std::move(comm_receiver),
        notif_receiver = std::move(notif_receiver)
    ] () mutable {
        prof_receiver.disconnect();
        comm_receiver.disconnect();
        notif_receiver.disconnect();
    });

    GracefulShutdown guard_;

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
