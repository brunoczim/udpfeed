#ifndef CLIENT_SOCKET_H_
#define CLIENT_SOCKET_H_ 1

#include "../shared/socket.h"
#include "../shared/rms.h"

class ClientSocket {
    private:
        class PendingResponse {
            public:
                Enveloped enveloped;
        };

        class PendingAck {
        };

        class Inner {
            public:
                Socket udp;
                uint64_t seqn;
                std::map<uint64_t, PendingResponse> pending_resps;
                std::map<uint64_t, PendingAck> pending_acks;
                SeqnSet sent_acks;
                RmGroup rms;
        };
};

#endif
