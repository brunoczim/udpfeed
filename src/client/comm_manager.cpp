#include "comm_manager.h"

void start_client_communication_manager(
    ThreadTracker& thread_tracker,
    ReliableSocket&& reliable_socket,
    Channel<std::shared_ptr<ClientOutputNotice>>::Sender&& to_interface,
    Channel<Enveloped>::Receiver&& from_session_man
)
{
}
