#include "shutdown.h"
#include <limits>
#include <iostream>
#include <thread>
#include <poll.h>

enum State {
    STATE_UNSET,
    STATE_SET,
    STATE_EXITING
};

static std::mutex control_mutex;
static std::condition_variable cond_var;
static std::atomic<State> state;

static void signal_handler(int signal_code);

void wait_for_graceful_shutdown(ShutdownEof shutdown_eof)
{
    using namespace std::chrono_literals;

    std::thread cin_thread;

    if (shutdown_eof == SHUTDOWN_ACTIVE_EOF) {
        cin_thread = std::move(std::thread([] {
            bool cancel = false;
            while (!cancel) {
                bool has_chars = false;
                while (!cancel && !has_chars) {
                    {
                        std::unique_lock lock(control_mutex);
                        if (state.load() == STATE_EXITING) {
                            cancel = true;
                        }
                    }
                    if (!cancel) {
                        struct pollfd fds[1];
                        fds[0].fd = fileno(stdin);
                        fds[0].events = POLLIN;
                        fds[0].revents = 0;
                        poll(fds, sizeof(fds) / sizeof(fds[0]), 10);
                        has_chars = (fds[0].revents & POLLIN) != 0;
                    }
                }
                if (has_chars) {
                    std::cin.ignore(
                        std::numeric_limits<std::streamsize>::max()
                    );
                }
                if (std::cin.eof()) {
                    std::unique_lock lock(control_mutex);
                    if (state.load() != STATE_EXITING) {
                        state.store(STATE_EXITING);
                        cond_var.notify_all();
                    }
                    cancel = true;
                }
            }
        }));
    }

    {
        std::unique_lock lock(control_mutex);
        if (state.load() == STATE_UNSET) {
            signal(SIGINT, signal_handler);
            state.store(STATE_SET);
        }

        while (state.load() != STATE_EXITING) {
            if (std::cin.eof()) {
                state.store(STATE_EXITING);
            } else {
                cond_var.wait_for(lock, 10ms);
            }
        }
    }

    if (shutdown_eof == SHUTDOWN_ACTIVE_EOF) {
        cin_thread.join();
    }
}

void signal_handler(int signal_code)
{
    state.store(STATE_EXITING);
}
