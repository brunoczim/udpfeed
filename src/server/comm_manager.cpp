#include <iostream>
#include "comm_manager.h"

void start_server_communication_manager(
    ThreadTracker& thread_tracker,
    ReliableSocket&& reliable_socket,
    Channel<ReliableSocket::ReceivedReq>::Sender&& to_profile_man,
    Channel<Enveloped>::Receiver&& from_notif_man
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
                Enveloped notif_enveloped = from_notif_man.receive();
                try {
                    ReliableSocket::SentReq sent_req =
                        socket->send_req(notif_enveloped);
                    Enveloped response = std::move(sent_req).receive_resp();

                    response.message.body->cast<MessageNotifyResp>();
                } catch (std::exception const& exc) {
                    std::cerr
                        << "error notifying connection "
                        << notif_enveloped.remote.to_string()
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
        to_profile_man = std::move(to_profile_man)
    ] () mutable {
        ReliableSocket::DisconnectGuard guard_(socket);

        try {
            for (;;) {
                ReliableSocket::ReceivedReq req = socket->receive_req();
                switch (req.req_enveloped().message.body->tag().type) {
                    case MSG_ERROR:
                        std::cerr
                            << "warning: received bad error request"
                            << std::endl;
                        break;

                    case MSG_DELIVER:
                        std::cerr
                            << "warning: received bad deliver request"
                            << std::endl;
                        break;

                    case MSG_CONNECT:
                    case MSG_DISCONNECT:
                    case MSG_FOLLOW:
                    case MSG_NOTIFY:
                        to_profile_man.send(req);
                        break;
                }
            }
        } catch (ChannelDisconnected const& exc) {
        }
    });
}
