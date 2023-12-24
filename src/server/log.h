#ifndef SERVER_LOG_H_
#define SERVER_LOG_H_ 1

#include <mutex>
#include <memory>
#include <functional>

class ServerLogger {
    private:
        static std::mutex control_mutex;
        static std::ostream* output;

    public:
        ServerLogger() = delete;

        static void set_output(std::ostream* output);

        template <typename F>
        static void with(F&& visitor);
};


template <typename F>
void ServerLogger::with(F&& visitor)
{
    std::function<void (std::ostream&)> visitor_function = std::move(visitor);
    std::unique_lock lock(ServerLogger::control_mutex);
    if (ServerLogger::output) {
        visitor_function(*ServerLogger::output);
    }
}

#endif
