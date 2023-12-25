#include <iostream>
#include "../shared/log.h"
#include "comm_manager.h"
#include "../shared/shutdown.h"

void start_server_communication_manager(
    ThreadTracker& thread_tracker,
    std::shared_ptr<ReliableSocket> const& socket,
    Channel<ReliableSocket::ReceivedReq>::Sender&& to_profile_man,
    Channel<Enveloped>::Receiver&& from_notif_man
)
{
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

                    response.message.body->cast<MessageDeliverResp>();
                } catch (std::exception const& exc) {
                    Logger::with([&exc, &notif_enveloped] (auto& output) {
                        output
                            << "error notifying connection "
                            << notif_enveloped.remote.to_string()
                            << " : "
                            << exc.what()
                            << std::endl;
                    });
                }
            }
        } catch (ChannelDisconnected const& exc) {
        }
        signal_graceful_shutdown();
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
                        Logger::with([] (auto& output) {
                            output
                                << "warning: received bad error request"
                                << std::endl;
                        });
                        break;

                    case MSG_DELIVER:
                        Logger::with([] (auto& output) {
                            output
                                << "warning: received bad deliver request"
                                << std::endl;
                        });
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
        signal_graceful_shutdown();
    });
}
