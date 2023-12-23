#ifndef CLIENT_SESSION_MANAGER_H_
#define CLIENT_SESSION_MANAGER_H_ 1

#include "../shared/socket.h"
#include "../shared/tracker.h"
#include "data.h"

class ClientSessionManDisconnectGuard {
    private:
        Address server_addr;
        std::shared_ptr<ReliableSocket> socket;
    public:
        ClientSessionManDisconnectGuard(
            Address server_addr,
            std::shared_ptr<ReliableSocket> const& socket
        );
        ~ClientSessionManDisconnectGuard();

        ReliableSocket const& operator*() const;
        ReliableSocket& operator*();

        ReliableSocket const* operator->() const;
        ReliableSocket* operator->();
};

void start_client_communication_manager(
    ThreadTracker& thread_tracker,
    Username const& username,
    Address server_addr,
    ReliableSocket&& reliable_socket,
    Channel<std::shared_ptr<ClientOutputNotice>>::Sender&& to_interface,
    Channel<std::shared_ptr<ClientInputCommand>>::Receiver&& from_interface
);

#endif
