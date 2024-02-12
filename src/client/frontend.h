#ifndef CLIENT_FRONTEND_H_
#define CLIENT_FRONTEND_H_ 1

#include "../shared/socket.h"
#include <set>

class ClientCouldNotConnect : public std::exception {
    public:
        virtual char const *what() const noexcept;
};

class ClientFrontend {
    private:
        class Inner {
            public:
                ReliableSocket socket;
                Username username;
                RmGroup rms;

                Inner(
                    ReliableSocket&& socket,
                    Username const& username,
                    std::set<Address> const& rm_lookup_addrs
                );

            private:
                bool try_connect(Address server_addr);
        };

    public:
        class SentReq {
            private:
                friend ClientFrontend;

                std::shared_ptr<ClientFrontend::Inner> inner;
                std::optional<ReliableSocket::SentReq> impl;
                Enveloped req_enveloped;

                SentReq(
                    std::shared_ptr<ClientFrontend::Inner> const& inner,
                    MessageBody const& request 
                );
                
                void send();

            public:
                Message const& req_message() const;

                Message receive_resp() &&;
        };

        class ReceivedReq {
            private:
                friend ClientFrontend;

                std::shared_ptr<ClientFrontend::Inner> inner;
                ReliableSocket::ReceivedReq impl;
                Enveloped req_enveloped_;

                ReceivedReq(
                    std::shared_ptr<ClientFrontend::Inner> const& inner,
                    ReliableSocket::ReceivedReq&& impl,
                    Enveloped req_enveloped
                );
            public:
                MessageBody const& req_enveloped() const;

                void send_resp(std::shared_ptr<MessageBody> const& response) &&;
        };

    private:
        std::shared_ptr<ClientFrontend::Inner> inner;

    public:
        ClientFrontend(
            ReliableSocket&& socket,
            Username const& username,
            std::set<Address> const& rm_lookup_addrs
        );

        SentReq send_req(MessageBody const& message);
        ReceivedReq receive_req();

        void disconnect();
        void disconnect_timeout(uint64_t interval_nanos, uint64_t intervals);
};

#endif
