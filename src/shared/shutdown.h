#ifndef SHARED_SHUTDOWN_H_
#define SHARED_SHUTDOWN_H_ 1

#include <csignal>
#include <functional>
#include <mutex>
#include <atomic>
#include <memory>
#include <condition_variable>

class GracefulShutdown {
    private:
        enum State {
            UNSET,
            SET,
            EXITING
        };

        static std::mutex control_mutex;
        static std::condition_variable cond_var;
        static std::atomic<State> state;
        static std::shared_ptr<std::function<void ()>> action;

        static void signal_handler(int signal_code);

        static void shutdown();

    public:
        template <typename F>
        static void set_action(F&& action);

        ~GracefulShutdown();
};


template <typename F>
void GracefulShutdown::set_action(F&& action)
{
    std::unique_lock lock(GracefulShutdown::control_mutex);
    GracefulShutdown::action = std::shared_ptr<std::function<void()>>(
        new std::function<void()>(std::move(action))
    );
    if (state.load() == GracefulShutdown::UNSET) {
        signal(SIGINT, GracefulShutdown::signal_handler);
        state.store(GracefulShutdown::SET);
    }
}

#endif
