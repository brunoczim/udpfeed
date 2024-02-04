#include "replication_manager.h"
#include "../shared/shutdown.h"

void start_server_replication_manager(
    ThreadTracker& thread_tracker,
    ServerGroup&& server_group,
    Channel<Enveloped>::Sender&& to_comm_man,
    Channel<ReliableSocket::ReceivedReq>::Receiver&& from_comm_man
)
{
    thread_tracker.spawn([group = std::move(server_group)] () mutable {
        try {
            for (;;) {
                if (!group.coordinator_addr()) {
                } else if (*group.coordinator_addr() == group.self_addr()) {
                } else {
                }
           }
        } catch (ChannelDisconnected const& exc) {
        }
        signal_graceful_shutdown();
    });
}
