#ifndef CLIENT_INTERFACE_H_
#define CLIENT_INTERFACE_H_ 1

#include "../shared/tracker.h"
#include "data.h"

void start_client_interface(
    ThreadTracker& thread_tracker,
    Channel<std::shared_ptr<ClientInputCommand>>::Sender&& to_session_man,
    Channel<std::shared_ptr<ClientOutputNotice>>::Receiver&& from_session_man 
);

#endif
