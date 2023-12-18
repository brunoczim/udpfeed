#ifndef SERVER_COMMUNICATION_H_
#define SERVER_COMMUNICATION_H_ 1

#include "../shared/socket.h"

class ServerCommunicationManager {
    private:
        ReliableSocket socket;
    public:
        ServerCommunicationManager(ReliableSocket&& socket);
};

#endif
