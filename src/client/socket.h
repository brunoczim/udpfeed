#ifndef CLIENT_SOCKET_H_
#define CLIENT_SOCKET_H_ 1

#include "../shared/socket.h"
#include "../shared/rms.h"

class ClientSocket {
    private:
        class Inner {
            public:
                Socket udp;
                uint64_t seqn;
                SeqnSet pending_response;
                SeqnSet pending_ack;
                SeqnSet ack_sent;
                RmGroup rms;
        };
};

#endif
