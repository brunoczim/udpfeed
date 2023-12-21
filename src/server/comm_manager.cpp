#include <iostream>
#include "comm_manager.h"

void start_server_communication_manager(
    ThreadTracker& thread_tracker,
    ReliableSocket&& reliable_socket,
    Channel<std::pair<Address, std::string>>::Receiver&& from_notif_man,
    Channel<ReliableSocket::ReceivedReq>::Sender&& to_notif_man,
    Channel<ReliableSocket::ReceivedReq>::Sender&& to_profile_man
)
{
    std::shared_ptr<ReliableSocket> socket = std::shared_ptr<ReliableSocket>(
        new ReliableSocket(std::move(reliable_socket))
    );

    thread_tracker.spawn([
        socket,
        from_notif_man = std::move(from_notif_man)
    ] () mutable {
        ReliableSocket::DisconnectGuard guard_(socket);

        try {
            for (;;) {
                std::pair<Address, std::string> notify_pair =
                    from_notif_man.receive();
                Address remote = std::get<0>(notify_pair);
                std::string const& notification = std::get<1>(notify_pair);

                Enveloped enveloped;
                enveloped.remote = remote;
                enveloped.message.body = std::shared_ptr<MessageBody>(
                    new MessageNotifyReq(notification)
                );

                try {
                    Enveloped response = 
                        std::move(socket->send_req(enveloped)).receive_resp();

                    response.message.body->cast<MessageNotifyResp>();
                } catch (std::exception const& exc) {
                    std::cerr
                        << "error notifying connection "
                        << remote.to_string()
                        << " : "
                        << exc.what()
                        << std::endl;
                }
            }
        } catch (ChannelDisconnected const& exc) {
        }
    });

    thread_tracker.spawn([
        socket,
        to_notif_man = std::move(to_notif_man),
        to_profile_man = std::move(to_profile_man)
    ] () mutable {
        ReliableSocket::DisconnectGuard guard_(socket);

        try {
            for (;;) {
                ReliableSocket::ReceivedReq req = socket->receive_req();
                switch (req.req_enveloped().message.body->tag().type) {
                    case MSG_ERROR:
                        std::cerr
                            << "warning, received bad error request"
                            << std::endl;
                            break;

                    case MSG_CONNECT:
                    case MSG_DISCONNECT:
                    case MSG_FOLLOW:
                        to_profile_man.send(req);
                        break;

                    case MSG_NOTIFY:
                        to_notif_man.send(req);
                        break;
                }
            }
        } catch (ChannelDisconnected const& exc) {
        }
    });
}