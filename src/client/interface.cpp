#include <iostream>
#include "interface.h"
#include "../shared/string_ext.h"

void start_client_interface(
    ThreadTracker& thread_tracker,
    Channel<std::shared_ptr<ClientInputCommand>>::Sender&& to_session_man,
    Channel<std::shared_ptr<ClientOutputNotice>>::Receiver&& from_session_man
)
{
    thread_tracker.spawn([
        to_session_man = std::move(to_session_man)
    ] () mutable {
        std::string follow_cmd = "FOLLOW";
        std::string send_cmd = "SEND";
        std::string line;
        while (!std::cin.fail()) {
            std::getline(std::cin, line);
            if (!std::cin.fail()) {
                line = trim_spaces(line);
                if (string_starts_with_ignore_case(line, follow_cmd)) {
                    try {
                        Username username = trim_spaces(line.substr(
                            follow_cmd.size(),
                            line.size() - follow_cmd.size()
                        ));
                        to_session_man.send(std::shared_ptr<ClientInputCommand>(
                            new ClientFollowCommand(username)
                        ));
                    } catch (InvalidUsername const& exc) {
                        std::cerr << exc.what() << std::endl;
                    }
                } else if (string_starts_with_ignore_case(line, send_cmd)) {
                    try {
                        NotifMessage message = trim_spaces(line.substr(
                            follow_cmd.size(),
                            line.size() - follow_cmd.size()
                        ));
                        to_session_man.send(std::shared_ptr<ClientInputCommand>(
                            new ClientSendCommand(message)
                        ));
                    } catch (InvalidNotifMessage const& exc) {
                        std::cerr << exc.what() << std::endl;
                    }
                } else if (line != "") {
                    std::cerr << "Unrecognized command." << std::endl;
                }
            }
        }
    });

    thread_tracker.spawn([
        from_session_man = std::move(from_session_man)
    ] () mutable {
        try {
            for (;;) {
                std::shared_ptr<ClientOutputNotice> notice =
                    from_session_man.receive();

                switch (notice->type()) {
                    case ClientOutputNotice::NOTIF: {
                        ClientNotifNotice const& notif_notice =
                            dynamic_cast<ClientNotifNotice const&>(*notice);
                        std::cerr
                            << notif_notice.sender.content()
                            << " sent: "
                            << notif_notice.message.content()
                            << std::endl;
                        break;
                    }
                    case ClientOutputNotice::ERROR: {
                        ClientErrorNotice const& error_notice =
                            dynamic_cast<ClientErrorNotice const&>(*notice);
                        std::cerr
                            << "recent UDP message resulted in error: "
                            << msg_error_render(error_notice.error)
                            << std::endl;
                        break;
                    }
                }
            }
        } catch (ChannelDisconnected const& exc) {
        }
    });
}
