#include "shutdown.h"
#include <iostream>

enum State {
    STATE_UNSET,
    STATE_SET,
    STATE_EXITING
};

static std::mutex control_mutex;
static std::condition_variable cond_var;
static std::atomic<State> state;

static void signal_handler(int signal_code);

void wait_for_graceful_shutdown()
{
    using namespace std::chrono_literals;

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

void signal_handler(int signal_code)
{
    state.store(STATE_EXITING);
}
