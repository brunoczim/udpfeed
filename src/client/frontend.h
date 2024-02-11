#ifndef CLIENT_FRONTEND_H_
#define CLIENT_FRONTEND_H_ 1

#include "../shared/socket.h"

class ClientFrontend {
    public:
        class SentReq {
            private:
                friend ClientFrontend;

                Enveloped req_enveloped_;

                ReliableSocket::SentReq impl;

                SentReq(
                    ReliableSocket::SentReq impl,
                    Channel<Enveloped>::Receiver&& channel
                );
            public:
                Enveloped const& req_enveloped() const;

                Enveloped receive_resp() &&;
        };

        class ReceivedReq {
            private:
                friend ClientFrontend;

                ReliableSocket::ReceivedReq impl;
                Enveloped req_enveloped_;

                ReceivedReq(
                    ReliableSocket::ReceivedReq impl,
                    Enveloped req_enveloped
                );
            public:
                Enveloped const& req_enveloped() const;

                void send_resp(std::shared_ptr<MessageBody> const& response) &&;
        };

    private:
        ReliableSocket socket;

    public:
        ClientFrontend(ReliableSocket&& socket);
};

#endif
