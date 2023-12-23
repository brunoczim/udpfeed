#include "prof_manager.h"

char const *InvalidServerProfManMsg::what() const noexcept
{
    return "invalid server profile manager message";
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
        try {
            for (;;) {
                ReliableSocket::ReceivedReq req = from_comm_man.receive();

                Enveloped req_enveloped = req.req_enveloped();

                try {
                    switch (req_enveloped.message.body->tag().type) {
                        case MSG_CONNECT: {
                            MessageConnectReq const& message = req_enveloped
                                .message
                                .body
                                ->cast<MessageConnectReq>();
                            profile_table->connect(
                                req_enveloped.remote,
                                message.username,
                                req_enveloped.message.header.timestamp
                            );
                            std::move(req).send_resp(
                                std::shared_ptr<MessageBody>(
                                    new MessageConnectResp
                                )
                            );
                            break;
                        }

                        case MSG_DISCONNECT:
                            profile_table->disconnect(
                                req_enveloped.remote,
                                req_enveloped.message.header.timestamp
                            );
                            std::move(req).send_resp(
                                std::shared_ptr<MessageBody>(
                                    new MessageDisconnectResp
                                )
                            );
                            break;

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
    });
}
