#include "session_manager.h"

void start_client_session_manager(
    ThreadTracker& thread_tracker,
    Username const& profile,
    Address server_addr,
    Channel<Enveloped>::Sender&& to_session_man,
    Channel<std::shared_ptr<ClientOutputNotice>>::Receiver&& from_interface
)
{
    thread_tracker.spawn([] {
        
    });
}
