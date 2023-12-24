#include "log.h"

std::mutex ServerLogger::control_mutex;

std::ostream* ServerLogger::output = nullptr;

void ServerLogger::set_output(std::ostream* output)
{
    std::unique_lock lock(ServerLogger::control_mutex);
    ServerLogger::output = output;
}
