#include "communication.h"

ServerCommunicationManager::ServerCommunicationManager(
    ReliableSocket&& socket
) :
    socket(std::move(socket))
{
}
