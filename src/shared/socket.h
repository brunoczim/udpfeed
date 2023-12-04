#ifndef SHARED_SOCKET_H_
#define SHARED_SOCKET_H_ 1

#include <stdexcept>
#include <cstdint>
#include <optional>
#include <functional>
#include <map>
#include "message.h"
#include "address.h"

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
        Socket(const Socket& obj) = delete;
        Socket& operator=(const Socket& obj) = delete;

        Message receive(Address &sender_addr_out);
        std::optional<Message> receive(
            int timeout_ms,
            Address &sender_addr_out
        );

        void send(Message const& message, Address const& receiver_addr);

        ~Socket();
};

class ReliableSocket {
    private:
        class PendingAck {
            public:
                Message message;
                uint16_t remaining_attempts;
                std::function<void (bool, Message)> send_handler;
        };

        class Connection {
            public:
                Address other_addr;
                uint64_t min_seqn_bound;
                std::map<uint64_t, PendingAck> pending_acks;
        };

        Socket udp;
        std::map<Address, Connection> connections;
        std::function<void (Message)> recv_handler;

    public:
        template <typename F>
        ReliableSocket(Socket&& udp, F&& recv_handler);

        void poll_recv();

        template <typename F>
        void send(
            Message const& message,
            Address const& receiver_addr,
            F&& handler
        );
};

#endif
