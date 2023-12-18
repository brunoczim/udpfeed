#include "communication.h"

ClientCommunicationManager::ClientCommunicationManager(
    ReliableSocket&& socket
) :
    socket(std::move(socket))
{
}
