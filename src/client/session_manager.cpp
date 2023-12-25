#include "session_manager.h"
#include "../shared/log.h"
#include "../shared/shutdown.h"

ClientSessionManDisconnectGuard::ClientSessionManDisconnectGuard(
    Address server_addr,
    std::shared_ptr<ReliableSocket> const& socket
) :
    server_addr(server_addr),
    socket(socket)
{
}

ClientSessionManDisconnectGuard::~ClientSessionManDisconnectGuard()
{
    ReliableSocket::DisconnectGuard guard_(this->socket);
    Enveloped disconnect_req;
    disconnect_req.remote = this->server_addr;
    disconnect_req.message.body = std::shared_ptr<MessageBody>(
        new MessageDisconnectReq
    );
    ReliableSocket::SentReq sent_req = this->socket->send_req(disconnect_req);
    try {
        std::move(sent_req).receive_resp();
    } catch (MissedResponse const& exc) {
    }
}

ReliableSocket const& ClientSessionManDisconnectGuard::operator*() const
{
    return *this->socket;
}

ReliableSocket& ClientSessionManDisconnectGuard::operator*()
{
    return *this->socket;
}

ReliableSocket const* ClientSessionManDisconnectGuard::operator->() const
{
    return this->socket.operator->();
}

ReliableSocket* ClientSessionManDisconnectGuard::operator->()
{
    return this->socket.operator->();
}

void start_client_session_manager(
    ThreadTracker& thread_tracker,
    Username const& username,
    Address server_addr,
    std::shared_ptr<ReliableSocket> const& socket,
    Channel<std::shared_ptr<ClientOutputNotice>>::Sender&& to_interface,
    Channel<std::shared_ptr<ClientInputCommand>>::Receiver&& from_interface
)
{
    Channel<std::shared_ptr<ClientOutputNotice>>::Sender moved_to_interface =
        std::move(to_interface);

    thread_tracker.spawn([
        username,
        from_interface,
        to_interface = moved_to_interface,
        server_addr,
        socket
    ] () mutable {
        ReliableSocket::DisconnectGuard raw_guard_(socket);
        try {
            Enveloped connect_req;
            connect_req.remote = server_addr;
            connect_req.message.body = std::shared_ptr<MessageBody>(
                new MessageConnectReq(username)
            );
            ReliableSocket::SentReq sent_connect_req =
                socket->send_req(connect_req);
            Enveloped connect_resp = std::move(sent_connect_req).receive_resp();
            connect_resp.message.body->cast<MessageConnectResp>();

            Logger::with([] (auto& output) {
                output << "Connected!" << std::endl;
            });

            ClientSessionManDisconnectGuard _guard(server_addr, socket);

            for (;;) {
                std::shared_ptr<ClientInputCommand> command =
                    from_interface.receive();
                
                try {
                    switch (command->type()) {
                        case ClientInputCommand::FOLLOW: {
                            ClientFollowCommand const& follow_cmd =
                                dynamic_cast<ClientFollowCommand const&>(
                                    *command
                                );
                            Enveloped cmd_req;
                            cmd_req.remote = server_addr;
                            cmd_req.message.body = std::shared_ptr<MessageBody>(
                                new MessageFollowReq(follow_cmd.username)
                            );
                            ReliableSocket::SentReq sent_req =
                                socket->send_req(cmd_req);
                            Enveloped resp = std::move(sent_req).receive_resp();
                            resp.message.body->cast<MessageFollowResp>();
                            break;
                        }
                        case ClientInputCommand::SEND: {
                            ClientSendCommand const& send_cmd =
                                dynamic_cast<ClientSendCommand const&>(
                                    *command
                                );
                            Enveloped cmd_req;
                            cmd_req.remote = server_addr;
                            cmd_req.message.body = std::shared_ptr<MessageBody>(
                                new MessageNotifyReq(send_cmd.message)
                            );
                            ReliableSocket::SentReq sent_req =
                                socket->send_req(cmd_req);
                            Enveloped resp = std::move(sent_req).receive_resp();
                            resp.message.body->cast<MessageNotifyResp>();
                            break;
                        }
                    }
                } catch (MissedResponse const& exc) {
                    to_interface.send(std::shared_ptr<ClientOutputNotice>(
                        new ClientErrorNotice(MSG_MISSED_RESP)
                    ));
                } catch (CastOnMessageError const& exc) {
                    to_interface.send(std::shared_ptr<ClientOutputNotice>(
                        new ClientErrorNotice(exc.error())
                    ));
                }
            }
        } catch (MissedResponse const& exc) {
            Logger::with([] (auto& output) {
                output << "!! Failed to connect !!" << std::endl;
            });
            try {
                to_interface.send(std::shared_ptr<ClientOutputNotice>(
                    new ClientErrorNotice(MSG_MISSED_RESP)
                ));
            } catch (ChannelDisconnected const& exc) {
            }
        } catch (ChannelDisconnected const& exc) {
        } catch (CastOnMessageError const& exc) {
            Logger::with([] (auto& output) {
                output << "!! Failed to connect !!" << std::endl;
            });
            try {
                to_interface.send(std::shared_ptr<ClientOutputNotice>(
                    new ClientErrorNotice(exc.error())
                ));
            } catch (ChannelDisconnected const& exc) {
            }
        }
        from_interface.disconnect();
        to_interface.disconnect();
        signal_graceful_shutdown();
    });

    thread_tracker.spawn([
        username,
        server_addr,
        to_interface = std::move(moved_to_interface),
        socket
    ] () mutable {
        bool connected = true;
        try {
            while (connected) {
                ReliableSocket::ReceivedReq received = socket->receive_req();
                Enveloped recv_enveloped = received.req_enveloped();
                try {
                    switch (recv_enveloped.message.body->tag().type) {
                        case MSG_CONNECT:
                        case MSG_ERROR:
                        case MSG_FOLLOW:
                        case MSG_NOTIFY:
                        {
                            std::shared_ptr<MessageBody> response =
                                std::shared_ptr<MessageBody>(
                                    new MessageErrorResp(MSG_BAD)
                                );
                            std::move(received).send_resp(response);
                            break;
                        }

                        case MSG_DELIVER: {
                            MessageDeliverReq const& msg_deliver =
                                recv_enveloped
                                    .message
                                    .body
                                    ->cast<MessageDeliverReq>();
                            std::shared_ptr<ClientOutputNotice> notice =
                                std::shared_ptr<ClientNotifNotice>(
                                    new ClientNotifNotice(
                                        msg_deliver.sender,
                                        msg_deliver.notif_message,
                                        msg_deliver.sent_at
                                    )
                                );
                            to_interface.send(notice);
                            std::shared_ptr<MessageBody> response =
                                std::shared_ptr<MessageBody>(
                                    new MessageDeliverResp
                                );
                            std::move(received).send_resp(response);
                            break;
                        }

                        case MSG_DISCONNECT: {
                            std::shared_ptr<MessageBody> response =
                                std::shared_ptr<MessageBody>(
                                    new MessageDisconnectResp
                                );
                            std::move(received).send_resp(response);
                            connected = false;
                            Logger::with([] (auto& output) {
                                output << "Server disconnected" << std::endl;
                            });
                            socket->disconnect();
                            break;
                        }
                    }
                } catch (CastOnMessageError const& exc) {
                    to_interface.send(std::shared_ptr<ClientOutputNotice>(
                        new ClientErrorNotice(exc.error())
                    ));
                }
            }
        } catch (ChannelDisconnected const& exc) {
        }
        signal_graceful_shutdown();
    });
}
