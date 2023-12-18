#include "communication.h"

ServerCommunicationManager::ServerCommunicationManager(
    ReliableSocket&& socket
) :
    socket(std::move(socket))
{
}

void ServerCommunicationManager::notify(
    Address remote,
    std::string const& notification
)
{
    Enveloped enveloped;
    enveloped.remote = remote;
    enveloped.message.body =
        std::shared_ptr<MessageBody>(new MessageNotifyReq(notification));
    std::move(this->socket.send_req(enveloped)).receive_resp();
}
