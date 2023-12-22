#include "prof_manager.h"

void start_server_profile_manager(
    ThreadTracker& thread_tracker,
    std::shared_ptr<ServerProfileTable> const& profile_table,
    Channel<ReliableSocket::ReceivedReq>::Receiver&& from_comm_man,
    Channel<Username>::Sender&& to_notif_man
)
{
}
