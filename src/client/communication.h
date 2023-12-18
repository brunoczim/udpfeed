#ifndef CLIENT_COMM_MANAGER_H_
#define CLIENT_COMM_MANAGER_H_ 1

#include "../shared/socket.h"

class ClientCommunicationManager {
    private:
        ReliableSocket socket;
    public:
        ClientCommunicationManager(ReliableSocket&& socket);
};

#endif
