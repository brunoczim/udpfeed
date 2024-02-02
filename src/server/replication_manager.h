#ifndef SERVER_REPLICATION_MANGER_H_
#define SERVER_REPLICATION_MANGER_H_ 1

#include <memory>

#include "../shared/channel.h"
#include "../shared/tracker.h"
#include "../shared/address.h"
#include "../shared/socket.h"
#include "group.h"

void start_server_replication_manager(
    ThreadTracker& thread_tracker,
    ServerGroup&& server_group,
    Channel<Enveloped>::Sender&& to_comm_man,
    Channel<ReliableSocket::ReceivedReq>::Receiver&& from_comm_man
);

#endif
