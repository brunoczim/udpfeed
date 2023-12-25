#ifndef SERVER_COMM_MANAGER_H_
#define SERVER_COMM_MANAGER_H_ 1

#include "../shared/socket.h"
#include "../shared/tracker.h"

void start_server_communication_manager(
    ThreadTracker& thread_tracker,
    std::shared_ptr<ReliableSocket> const& socket,
    Channel<ReliableSocket::ReceivedReq>::Sender&& to_profile_man,
    Channel<Enveloped>::Receiver&& from_notif_man
);

#endif
