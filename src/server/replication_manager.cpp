#include "replication_manager.h"

static bool try_connect(Address remote_server, ServerGroup& group);

void start_server_replication_manager(
    ThreadTracker& thread_tracker,
    std::shared_ptr<ServerGroup> const& server_group,
    Channel<Enveloped>::Sender&& to_comm_man,
    Channel<ReliableSocket::ReceivedReq>::Receiver&& from_comm_man
)
{
}
