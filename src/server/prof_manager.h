#ifndef SERVER_PROF_MANAGER_H_
#define SERVER_PROF_MANAGER_H_ 1

#include <exception>
#include "../shared/socket.h"
#include "../shared/tracker.h"
#include "../shared/username.h"
#include "data.h"

class InvalidServerProfManMsg {
    public:
        virtual char const *what() const noexcept;
};

void start_server_profile_manager(
    ThreadTracker& thread_tracker,
    std::shared_ptr<ServerProfileTable> const& profile_table,
    Channel<Enveloped>::Sender&& to_comm_man,
    Channel<ReliableSocket::ReceivedReq>::Receiver&& from_comm_man,
    Channel<Username>::Sender&& to_notif_man
);

#endif
