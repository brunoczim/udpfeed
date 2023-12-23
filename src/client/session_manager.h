#ifndef CLIENT_SESSION_MANAGER_H_
#define CLIENT_SESSION_MANAGER_H_ 1

#include "../shared/tracker.h"
#include "../shared/address.h"
#include "data.h"

void start_client_session_manager(
    ThreadTracker& thread_tracker,
    Username const& profile,
    Address server_addr,
    Channel<Enveloped>::Sender&& to_session_man,
    Channel<std::shared_ptr<ClientOutputNotice>>::Receiver&& from_interface
);

#endif
