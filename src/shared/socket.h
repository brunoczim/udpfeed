#ifndef SHARED_SOCKET_H_
#define SHARED_SOCKET_H_ 1

#include <stdexcept>
#include <cstdint>
#include <optional>
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
        Socket(const Socket& obj) = delete;
        Socket& operator=(const Socket& obj) = delete;

        Message receive(Address &sender_addr_out);
        std::optional<Message> receive(
            int timeout_ms,
            Address &sender_addr_out
        );

        void send(Message const& message, Address const& receiver_addr);
        void send(
            std::shared_ptr<MessageBody> const& message_body,
            Address const& receiver_addr
        );

        ~Socket();
};

#endif
