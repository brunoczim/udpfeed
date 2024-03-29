#include "prof_manager.h"
#include "../shared/log.h"
#include "../shared/shutdown.h"

char const *InvalidServerProfManMsg::what() const noexcept
{
    return "invalid server profile manager message";
}

ServerProfManDataGuard::ServerProfManDataGuard(
    std::shared_ptr<ServerProfileTable> const& profile_table
) :
    profile_table(profile_table)
{
}

ServerProfManDataGuard::~ServerProfManDataGuard()
{
    this->profile_table->shutdown();
}

void start_server_profile_manager(
    ThreadTracker& thread_tracker,
    std::shared_ptr<ServerProfileTable> const& profile_table,
    Channel<Username>::Sender&& to_notif_man,
    Channel<ReliableSocket::ReceivedReq>::Receiver&& from_comm_man
)
{
    thread_tracker.spawn([
        profile_table,
        from_comm_man = std::move(from_comm_man),
        to_notif_man = std::move(to_notif_man)
    ] () mutable {
        ServerProfManDataGuard shutdown_guard_(profile_table);
        try {
            for (;;) {
                ReliableSocket::ReceivedReq req = from_comm_man.receive();

                Enveloped req_enveloped = req.req_enveloped();

                try {
                    switch (req_enveloped.message.body->tag().type) {
                        case MSG_CLIENT_CONN: {
                            MessageClientConnReq const& message = req_enveloped
                                .message
                                .body
                                ->cast<MessageClientConnReq>();
                            profile_table->connect(
                                req_enveloped.remote,
                                message.username,
                                req_enveloped.message.header.timestamp
                            );
                            std::move(req).send_resp(
                                std::shared_ptr<MessageBody>(
                                    new MessageClientConnResp
                                )
                            );
                            Logger::with([
                                &message,
                                &req_enveloped
                            ] (auto& output) {
                                output << "Client "
                                    << req_enveloped.remote.to_string()
                                    << " connected with username "
                                    << message.username.content()
                                    << " at "
                                    << req_enveloped.message.header.timestamp
                                    << std::endl;
                            });
                            break;
                        }

                        case MSG_DISCONNECT: {
                            bool disconnected = profile_table->disconnect(
                                req_enveloped.remote,
                                req_enveloped.message.header.timestamp
                            );
                            std::move(req).send_resp(
                                std::shared_ptr<MessageBody>(
                                    new MessageDisconnectResp
                                )
                            );
                            if (disconnected) {
                                Logger::with([
                                    &req_enveloped
                                ] (auto& output) {
                                    int64_t timestamp =
                                        req_enveloped.message.header.timestamp;
                                    output << "Client "
                                        << req_enveloped.remote.to_string()
                                        << " disconnected at "
                                        << timestamp
                                        << std::endl;
                                });
                            }
                            break;
                        }

                        case MSG_FOLLOW: {
                            MessageFollowReq const& message = req_enveloped
                                .message
                                .body
                                ->cast<MessageFollowReq>();
                            profile_table->follow(
                                req_enveloped.remote,
                                message.username,
                                req_enveloped.message.header.timestamp
                            );
                            std::move(req).send_resp(
                                std::shared_ptr<MessageBody>(
                                    new MessageFollowResp
                                )
                            );
                            Logger::with([
                                &message,
                                &req_enveloped
                            ] (auto& output) {
                                output << "Client "
                                    << req_enveloped.remote.to_string()
                                    << " profile started following "
                                    << message.username.content()
                                    << " at "
                                    << req_enveloped.message.header.timestamp
                                    << std::endl;
                            });
                            break;
                        }

                        case MSG_NOTIFY: {
                            MessageNotifyReq const& message = req_enveloped
                                .message
                                .body
                                ->cast<MessageNotifyReq>();
                            profile_table->notify(
                                req_enveloped.remote,
                                message.notif_message,
                                to_notif_man,
                                req_enveloped.message.header.timestamp
                            );
                            std::move(req).send_resp(
                                std::shared_ptr<MessageBody>(
                                    new MessageNotifyResp
                                )
                            );
                            Logger::with([
                                &message,
                                &req_enveloped
                            ] (auto& output) {
                                output << "Client "
                                    << req_enveloped.remote.to_string()
                                    << " sent notification at "
                                    << req_enveloped.message.header.timestamp
                                    << " with: "
                                    << message.notif_message.content()
                                    << std::endl;
                            });
                            break;
                        }

                        default:
                            throw InvalidServerProfManMsg();
                    }
                } catch (ThrowableMessageError exc) {
                    std::move(req).send_resp(std::shared_ptr<MessageBody>(
                        new MessageErrorResp(exc.error())
                    ));
                }
            }
        } catch (ChannelDisconnected const& exc) {
        }
        signal_graceful_shutdown();
    });

    thread_tracker.spawn([profile_table] () mutable {
        while (profile_table->persist_on_dirty()) {}
    });
}
