#include "log.h"

std::mutex Logger::control_mutex;

std::ostream* Logger::output = nullptr;

void Logger::set_output(std::ostream* output)
{
    std::unique_lock lock(Logger::control_mutex);
    Logger::output = output;
}
