#ifndef CLIENT_SOCKET_H_
#define CLIENT_SOCKET_H_ 1

#include "../shared/socket.h"
#include "../shared/rms.h"
#include "../shared/cooldown.h"

class ClientSocket {
    public:
        class Config {
            public:
                BinaryExpCooldown::Config pending_resps_cooldown;
                BinaryExpCooldown::Config pending_ack_cooldown;
                LinearCooldown::Config startup_cooldown;
        };

    private:
        class PendingResponse {
            public:
                Enveloped req_enveloped;
                BinaryExpCooldown cooldown;
        };

        class PendingAck {
            public:
                Enveloped resp_enveloped;
                BinaryExpCooldown cooldown;
        };

        class Inner {
            public:
                std::mutex mutex;

                ClientSocket::Config config;

                Socket udp;
                uint64_t seqn;

                std::map<uint64_t, PendingResponse> pending_resps;
                std::map<uint64_t, PendingAck> pending_acks;
                SeqnSet sent_acks;

                RmGroup rms;

                Inner(Socket&& udp, ClientSocket::Config const& config);
        };

    public:
        class SentReq {
            private:
                std::shared_ptr<ClientSocket::Inner> inner;
                Enveloped sent_enveloped;
                Channel<Enveloped>::Receiver callback;
        };

        class ReceivedReq {
            private:
                std::shared_ptr<ClientSocket::Inner> inner;
                Enveloped received_enveloped;
        };

        ClientSocket(ClientSocket::Config const& config);

    private:
        ClientSocket(
            std::shared_ptr<Inner> const& inner,
            ClientSocket::Config const& config
        );
};

#endif
