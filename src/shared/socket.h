#ifndef SHARED_SOCKET_H_
#define SHARED_SOCKET_H_ 1

#include <stdexcept>
#include <cstdint>
#include <optional>
#include <functional>
#include <map>
#include <thread>
#include "message.h"
#include "address.h"
#include "channel.h"

class SocketError : public std::exception {};

class SocketIoError : public SocketError {
    private:
        std::string full_message;
        int c_errno_;
        std::string message;

    public:
        SocketIoError(std::string const& message);
        int c_errno() const;
        virtual const char *what() const noexcept;
};

class InvalidAddrLen : public SocketError {
    private:
        std::string message;

    public:
        InvalidAddrLen(std::string const& message);
        virtual const char *what() const noexcept;
};

class Socket {
    private:
        int sockfd;
        size_t max_message_size;

    public:
        Socket(size_t max_message_size);
        Socket(size_t max_message_size, uint16_t port);
        Socket(Socket&& other);
        Socket(Socket const& obj) = delete;
        Socket& operator=(Socket const& obj) = delete;
        Socket& operator=(Socket&& obj);

        ~Socket();

        Message receive(Address &sender_addr_out);
        std::optional<Message> receive(
            int timeout_ms,
            Address &sender_addr_out
        );

        void send(Message const& message, Address const& receiver_addr);
    
    private:
        void close();
};

class ReliableSocket {
    public:
        class ReqSent {
            private:
                friend ReliableSocket;

                Channel<Message>::Receiver channel;

                ReqSent(Channel<Message>::Receiver&& channel);
            public:
                Message receive_response() &&;
        };

        class ReceivedReq {
            private:
                friend ReliableSocket;

                Message message;

                Channel<Message>::Sender channel;

                ReceivedReq(
                    Channel<Message>::Sender&& channel,
                    Message req_message
                );
            public:
                Message const& req_message() const;

                void send_response(Message response) &&;
        };

    private:
        class PendingResponse {
            public:
                Message request;
                uint64_t remaining_attempts;
        };

        class Connection {
            public:
                Address remote_address;
                uint64_t max_attemtps;
                uint64_t max_cached_responses;
                uint64_t min_accepted_req_seqn;
                std::queue<Message> cached_responses;
                std::map<uint64_t, PendingResponse> pending_responses;
        };

        class Inner {
            private:
                Socket udp;
                uint64_t max_req_attempt;
                std::map<Address, Connection> connection;
            public:
                Inner(Socket&& udp, uint64_t max_req_attempt);
        };

        std::shared_ptr<Inner> inner;

        std::thread send_thread;
        std::thread receive_thread;

        Channel<std::pair<Message, Channel<Message>::Sender>>::Sender
            send_channel;

        Channel<std::pair<Message, Channel<Message>::Sender>>::Receiver
            receive_channel;

        ReliableSocket(
            Channel<std::pair<Message, Channel<Message>::Sender>>&&
                send_channel,

            Channel<std::pair<Message, Channel<Message>::Sender>>&&
                receive_channel,

            std::shared_ptr<Inner> inner
        );

    public:
        ReliableSocket(Socket&& udp, uint64_t max_req_attempt);

        ReliableSocket::ReqSent send_req(Message message);
        ReliableSocket::ReceivedReq receive_req();

        ~ReliableSocket();
};

#endif
