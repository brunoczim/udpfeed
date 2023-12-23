#ifndef CLIENT_COMM_MANAGER_H_
#define CLIENT_COMM_MANAGER_H_ 1

#include "../shared/socket.h"
#include "../shared/tracker.h"
#include "data.h"

void start_client_communication_manager(
    ThreadTracker& thread_tracker,
    ReliableSocket&& reliable_socket,
    Channel<std::shared_ptr<ClientOutputNotice>>::Sender&& to_interface,
    Channel<Enveloped>::Receiver&& from_session_man
);

#endif
