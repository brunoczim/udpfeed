#include "replication_manager.h"

void start_server_replication_manager(
    ThreadTracker& thread_tracker,
    Address self,
    Channel<Enveloped>::Sender&& to_comm_man,
    Channel<ReliableSocket::ReceivedReq>::Receiver&& from_comm_man
)
{

}
