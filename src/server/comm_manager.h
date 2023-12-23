#ifndef SERVER_COMMUNICATION_H_
#define SERVER_COMMUNICATION_H_ 1

#include "../shared/socket.h"
#include "../shared/tracker.h"

void start_server_communication_manager(
    ThreadTracker& thread_tracker,
    ReliableSocket&& reliable_socket,
    Channel<ReliableSocket::ReceivedReq>::Sender&& to_profile_man,
    Channel<Enveloped>::Receiver&& from_notif_man
);

#endif
