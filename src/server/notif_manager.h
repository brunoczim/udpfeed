#ifndef SERVER_NOTIF_MANAGER_H_
#define SERVER_NOTIF_MANAGER_H_ 1

#include "../shared/message.h"
#include "../shared/tracker.h"
#include "../shared/username.h"
#include "data.h"

void start_server_notification_manager(
    ThreadTracker& thread_tracker,
    std::shared_ptr<ServerProfileTable> const& profile_table,
    Channel<Enveloped>::Sender&& to_comm_man,
    Channel<Username>::Receiver&& from_prof_man
);

#endif
