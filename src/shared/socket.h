#ifndef SHARED_SOCKET_H_
#define SHARED_SOCKET_H_ 1

#include <stdexcept>
#include <cstdint>
#include <optional>
#include <functional>
#include <map>
#include <set>
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

        Envelope receive();
        std::optional<Envelope> receive(int timeout_ms);

        void send(Envelope const& envelope);
    
    private:
        void close();
};

/**
 * # Problem
 *
 *  - Need to receive messages and pass them forward quickly.
 *  - Need to periodically check requests and re-send them.
 *
 * # Solution
 *
 *  ## Threads
 *
 * Three extra threads:
 * 
 *  1. "input": receives messages and quickly pass them forward
 *  2. "handler": gets received messages and process them
 *  3. "bumper": periodically checks for requests not responded and resends them 
 *
 *  User sends directly.
 *
 *  ## Communication
 *
 *  - input sends messages to handler
 *  - handler sends requests to users through 'receive_req'
 *  - handler sends responses to users through 'receive_resp'
 *  - users sends requests directly with callback through 'send_req'
 *  - users sends responses directly without callback through 'send_resp'
 */
class ReliableSocket {
    private:
        class PendingResponse {
            public:
                Envelope request;
                uint64_t remaining_attempts;
                Channel<Envelope>::Sender callback;
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
                std::set<std::pair<Address, uint64_t>> pending_response_keys;
                std::map<Address, Connection> connection;
                Channel<Envelope>::Receiver handler_to_req_receiver;

            public:
                Inner(
                    Socket&& udp,
                    uint64_t max_req_attempt,
                    Channel<Envelope>::Receiver&& handler_to_req_receiver
                );
        };

    public:
        class SentReq {
            private:
                friend ReliableSocket;

                Channel<Envelope>::Receiver channel;

                SentReq(Channel<Envelope>::Receiver&& channel);
            public:
                Envelope receive_resp() &&;
        };

        class ReceivedReq {
            private:
                friend ReliableSocket;

                std::shared_ptr<Inner> inner;
                Envelope req_envelope_;

                ReceivedReq(
                    std::shared_ptr<Inner> const& inner,
                    Envelope req_envelope
                );
            public:
                Envelope const& req_envelope() const;

                void send_resp(Message response) &&;
        };

    private:
        std::shared_ptr<Inner> inner;
        std::thread input_thread;
        std::thread handler_thread;
        std::thread bumper_thread;

        ReliableSocket(
            std::shared_ptr<ReliableSocket::Inner> inner,
            Channel<Envelope>&& input_to_handler_channel,
            Channel<Envelope>::Sender&& handler_to_req_receiver
        );

        ReliableSocket(
            Socket&& udp,
            uint64_t max_req_attempt,
            Channel<Envelope>&& input_to_handler_channel,
            Channel<Envelope>&& handler_to_recv_req_channel
        );

    public:
        ReliableSocket(Socket&& udp, uint64_t max_req_attempt);

        SentReq send_req(Envelope message);
        ReceivedReq receive_req();

        ~ReliableSocket();
};

#endif
