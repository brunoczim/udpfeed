#include "notif_manager.h"

void start_server_notification_manager(
    ThreadTracker& thread_tracker,
    std::shared_ptr<ServerProfileTable> const& profile_table,
    Channel<Enveloped>::Sender&& to_comm_man,
    Channel<Username>::Receiver&& from_prof_man
)
{
    thread_tracker.spawn([
        profile_table,
        to_comm_man = std::move(to_comm_man),
        from_prof_man = std::move(from_prof_man)
    ] () mutable {
        try {
            for (;;) {
                Username to_be_notif_username = from_prof_man.receive();
                std::optional<PendingNotif> maybe_pending_notif =
                    profile_table->consume_one_notif(to_be_notif_username);

                if (auto pending_notif = maybe_pending_notif) {
                    Enveloped enveloped;
                    enveloped.message.body = std::shared_ptr<MessageBody>(
                        new MessageNotifyReq(
                            Username(pending_notif->sender),
                            NotifMessage(pending_notif->message)
                        )
                    );

                    for (Address receiver : pending_notif->receivers) {
                        enveloped.remote = receiver;
                        to_comm_man.send(enveloped);
                    }
                }
            }
        } catch (ChannelDisconnected const& exc) {
        }
    });
}
