#include "replication_manager.h"

static bool try_connect(Address remote_server, ServerGroup& group);

void start_server_replication_manager(
    ThreadTracker& thread_tracker,
    Address self,
    Channel<Enveloped>::Sender&& to_comm_man,
    Channel<ReliableSocket::ReceivedReq>::Receiver&& from_comm_man
)
{
    ServerGroup group(self);
    std::set server_addrs = ServerGroup::load_server_list();

    bool connected = false;

    for (auto server_addr : server_addrs) {
        connected = try_connect(server_addr, group);
        if (connected) {
            break;
        }
    }

    if (!connected) {
        try_connect_broadcast(
    }
}
