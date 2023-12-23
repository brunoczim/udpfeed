#include "shutdown.h"
#include <iostream>

std::mutex GracefulShutdown::control_mutex;
std::condition_variable GracefulShutdown::cond_var;
std::atomic<GracefulShutdown::State> GracefulShutdown::state;
std::shared_ptr<std::function<void ()>> GracefulShutdown::action;

GracefulShutdown::~GracefulShutdown()
{
    using namespace std::chrono_literals;

    std::unique_lock lock(GracefulShutdown::control_mutex);
    while (GracefulShutdown::state.load() != GracefulShutdown::EXITING) {
        if (std::cin.eof()) {
            GracefulShutdown::shutdown();
        } else {
            GracefulShutdown::cond_var.wait_for(lock, 10ms);
        }
    }
}

void GracefulShutdown::shutdown()
{
    GracefulShutdown::State previous =
        GracefulShutdown::state.exchange(GracefulShutdown::EXITING);
    if (previous != GracefulShutdown::EXITING) {
        (*GracefulShutdown::action)();
    }
}

void GracefulShutdown::signal_handler(int signal_code)
{
    shutdown();
}
