#ifndef SHARED_LOG_H_
#define SHARED_LOG_H_ 1

#include <mutex>
#include <memory>
#include <functional>

class Logger {
    private:
        static std::mutex control_mutex;
        static std::ostream* output;

    public:
        Logger() = delete;

        static void set_output(std::ostream* output);

        template <typename F>
        static void with(F&& visitor);
};


template <typename F>
void Logger::with(F&& visitor)
{
    std::function<void (std::ostream&)> visitor_function = std::move(visitor);
    std::unique_lock lock(Logger::control_mutex);
    if (Logger::output) {
        visitor_function(*Logger::output);
    }
}

#endif
