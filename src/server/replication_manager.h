#ifndef SERVER_REPLICATION_MANGER_H_
#define SERVER_REPLICATION_MANGER_H_ 1

#include "../shared/message.h"
#include "../shared/tracker.h"
#include "../shared/address.h"
#include "../shared/socket.h"

void start_server_replication_manager(
    ThreadTracker& thread_tracker,
    Address self,
    Channel<Enveloped>::Sender&& to_comm_man,
    Channel<ReliableSocket::ReceivedReq>::Receiver&& from_comm_man
);

#endif
