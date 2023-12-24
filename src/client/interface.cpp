#include <iostream>
#include <poll.h>
#include "interface.h"
#include "../shared/string_ext.h"
#include "../shared/log.h"
#include "../shared/shutdown.h"

static std::mutex stdout_mutex;

void start_client_interface(
    ThreadTracker& thread_tracker,
    Channel<std::shared_ptr<ClientInputCommand>>::Sender&& to_session_man,
    Channel<std::shared_ptr<ClientOutputNotice>>::Receiver&& from_session_man
)
{
    thread_tracker.spawn([
        to_session_man = std::move(to_session_man)
    ] () mutable {
        std::string follow_cmd = "FOLLOW ";
        std::string send_cmd = "SEND ";
        std::string line;
        while (!std::cin.eof() && to_session_man.is_connected()) {
            bool has_chars = false;
            while (
                !std::cin.eof()
                && to_session_man.is_connected()
                && !has_chars
            ) {
                struct pollfd fds[1];
                fds[0].fd = fileno(stdin);
                fds[0].events = POLLIN;
                fds[0].revents = 0;
                poll(fds, sizeof(fds) / sizeof(fds[0]), 10);
                has_chars = (fds[0].revents & POLLIN) != 0;
            }

            if (has_chars) {
                std::getline(std::cin, line);
                if (!std::cin.bad() && !std::cin.fail() && !std::cin.eof()) {
                    line = trim_spaces(line);
                    if (string_starts_with_ignore_case(line, follow_cmd)) {
                        try {
                            Username username = trim_spaces(line.substr(
                                follow_cmd.size(),
                                line.size() - follow_cmd.size()
                            ));
                            to_session_man
                                .send(std::shared_ptr<ClientInputCommand>(
                                    new ClientFollowCommand(username)
                                ));
                        } catch (InvalidUsername const& exc) {
                            std::cerr << exc.what() << std::endl;
                        }
                    } else if (string_starts_with_ignore_case(line, send_cmd)) {
                        try {
                            NotifMessage message = trim_spaces(line.substr(
                                send_cmd.size(),
                                line.size() - send_cmd.size()
                            ));
                            to_session_man
                                .send(std::shared_ptr<ClientInputCommand>(
                                    new ClientSendCommand(message)
                                ));
                        } catch (InvalidNotifMessage const& exc) {
                            Logger::with([&exc] (auto& output) {
                                output << exc.what() << std::endl;
                            });
                        }
                    } else if (line != "") {
                        std::unique_lock lock(stdout_mutex);
                        std::cout << "Unrecognized command." << std::endl;
                    }
                }
            }
        }
        signal_graceful_shutdown();
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
                        std::unique_lock lock(stdout_mutex);
                        ClientNotifNotice const& notif_notice =
                            dynamic_cast<ClientNotifNotice const&>(*notice);
                        std::cout
                            << notif_notice.sender.content()
                            << " sent at "
                            << notif_notice.sent_at
                            << ": "
                            << notif_notice.message.content()
                            << std::endl;
                        break;
                    }
                    case ClientOutputNotice::ERROR: {
                        ClientErrorNotice const& error_notice =
                            dynamic_cast<ClientErrorNotice const&>(*notice);

                        Logger::with([&error_notice] (auto& output) {
                            output
                                << "recent UDP message resulted in error: "
                                << msg_error_render(error_notice.error)
                                << std::endl;
                        });
                        break;
                    }
                }
            }
        } catch (ChannelDisconnected const& exc) {
        }
    });
}
