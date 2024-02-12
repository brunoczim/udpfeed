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
                BinaryExpCooldown::Config startup_cooldown;
                LinearCooldown::Config ping_cooldown;

                uint64_t bump_interval_nanos;

                int poll_timeout_ms;

                Config();
        };

    private:
        class PendingResponse {
            public:
                Enveloped req_enveloped;
                BinaryExpCooldown cooldown;
                std::optional<Channel<Enveloped>::Sender> callback;

                PendingResponse(
                    Enveloped const& req_enveloped,
                    ClientSocket::Config const& config,
                    std::optional<Channel<Enveloped>::Sender>&& callback
                );
        };

        class PendingAck {
            public:
                Enveloped resp_enveloped;
                BinaryExpCooldown cooldown;
        };

        class Inner {
            public:
                std::mutex net_control_mutex;

                ClientSocket::Config config;

                Socket udp;
                uint64_t seqn;

                std::map<uint64_t, PendingResponse> pending_resps;
                std::map<uint64_t, PendingAck> pending_acks;
                SeqnSet sent_acks;

                std::set<Address> rms;
                std::optional<Address> primary_rm;

                Channel<Enveloped>::Receiver handler_to_req_receiver;

                Inner(
                    Socket&& udp,
                    ClientSocket::Config const& config,
                    Channel<Enveloped>::Receiver&& handler_to_req_receiver
                );

                bool is_connected();

                std::optional<Enveloped> receive_raw(int poll_timeout_ms);

                std::optional<Enveloped> handle(Enveloped enveloped);

                std::vector<Enveloped> bump();
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

        std::shared_ptr<ClientSocket::Inner> inner;
        std::thread input_thread;
        std::thread handler_thread;
        std::thread bumper_thread;

        ClientSocket(
            Socket&& udp,
            ClientSocket::Config const& config = ClientSocket::Config()
        );

    private:
        ClientSocket(
            Socket&& udp,
            ClientSocket::Config const& config,
            Channel<Enveloped>&& input_to_handler_channel,
            Channel<Enveloped>&& handler_to_req_channel
        );

        ClientSocket(
            std::shared_ptr<ClientSocket::Inner> const& inner
            Channel<Enveloped>&& input_to_handler_channel,
            Channel<Enveloped>::Sender&& handler_to_req_receiver
        );
};

#endif
