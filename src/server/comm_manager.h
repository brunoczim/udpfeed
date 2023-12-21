#ifndef SERVER_COMMUNICATION_H_
#define SERVER_COMMUNICATION_H_ 1

#include "../shared/socket.h"
#include "../shared/tracker.h"

void start_server_communication_manager(
    ThreadTracker& thread_tracker,
    ReliableSocket&& reliable_socket,
    Channel<std::pair<Address, std::string>>::Receiver&& from_notif_man,
    Channel<ReliableSocket::ReceivedReq>::Sender&& to_notif_man,
    Channel<ReliableSocket::ReceivedReq>::Sender&& to_profile_man
);

#endif
