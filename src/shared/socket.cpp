#include "socket.h"
#include <cerrno>
#include <cstring>
#include <unistd.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <iostream>
#include <sstream>

SocketIoError::SocketIoError(std::string const& message) :
    c_errno_(errno),
    message(message),
    full_message(message)
{
    this->full_message += ": ";
    this->full_message += strerror(errno);
}

int SocketIoError::c_errno() const
{
    return this->c_errno_;
}

const char *SocketIoError::what() const noexcept
{
    return this->full_message.c_str();
}

InvalidAddrLen::InvalidAddrLen(std::string const& message) :
    message(message)
{
}

const char *InvalidAddrLen::what() const noexcept
{
    return this->message.c_str();
}

Socket::Socket(size_t max_message_size) : max_message_size(max_message_size)
{
    this->sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (this->sockfd < 0) {
        throw SocketIoError("socket create");
    }
}

Socket::Socket(size_t max_message_size, uint16_t port) :
    Socket(max_message_size)
{
    struct sockaddr_in bind_addr;

    bind_addr.sin_family = AF_INET;
    bind_addr.sin_port = htons(port);
    bind_addr.sin_addr.s_addr = INADDR_ANY;
    bzero(&bind_addr.sin_zero, 8);

    int status = bind(
        this->sockfd,
        (struct sockaddr *) &bind_addr,
        sizeof(bind_addr)
    );
    if (status < 0) {
        throw SocketIoError("socket bind");
    }
}

Socket::Socket(Socket&& other) : sockfd(other.sockfd)
{
    other.sockfd = -1;
}

Socket& Socket::operator=(Socket&& obj)
{
    if (this->sockfd != obj.sockfd) {
        this->close();
    }
    this->sockfd = obj.sockfd;
    obj.sockfd = -1;
    return *this;
}

Socket::~Socket()
{
    this->close();
}

Enveloped Socket::receive()
{
    struct sockaddr_in sender_addr;
    socklen_t sender_len = sizeof(sender_addr);
    std::string buf(this->max_message_size + 1, '\0');
    ssize_t count = recvfrom(
        this->sockfd,
        buf.data(),
        this->max_message_size,
        0,
        (struct sockaddr *) &sender_addr,
        &sender_len
    );
    if (count < 0) {
        throw SocketIoError("socket recv");
    }
    if (sender_len != sizeof(sender_addr)) {
        throw InvalidAddrLen(
            "Socket address unexpectedly has the wrong length"
        );
    }

    Enveloped enveloped;

    enveloped.remote.ipv4 = ntohl(sender_addr.sin_addr.s_addr);
    enveloped.remote.port = ntohs(sender_addr.sin_port);

    buf.resize(count);
    std::istringstream istream(buf);
    PlaintextDeserializer deserializer_impl(istream);
    Deserializer& deserializer = deserializer_impl;
    deserializer >> enveloped.message;

    return enveloped;
}

std::optional<Enveloped> Socket::receive(int timeout_ms)
{
    struct pollfd fds[1];
    fds[0].fd = this->sockfd;
    fds[0].events = POLLIN;

    int status = poll(fds, sizeof(fds) / sizeof(fds[0]), timeout_ms);
    if (status < 0) {
        throw SocketIoError("socket poll");
    }
    if (fds[0].revents & POLLIN) {
        return this->receive();
    }
    return std::optional<Enveloped>();
}

void Socket::send(Enveloped const& enveloped)
{
    std::ostringstream ostream;
    PlaintextSerializer serializer_impl(ostream);
    Serializer& serializer = serializer_impl;
    serializer << enveloped.message;
    std::string buf = ostream.str();
    
    struct sockaddr_in receiver_addr_in;

    receiver_addr_in.sin_family = AF_INET;
    receiver_addr_in.sin_port = htons(enveloped.remote.port);
    receiver_addr_in.sin_addr.s_addr = htonl(enveloped.remote.ipv4);
    bzero(&receiver_addr_in.sin_zero, 8);

    ssize_t result = sendto(
        this->sockfd,
        buf.data(),
        buf.size(),
        0,
        (struct sockaddr *) &receiver_addr_in,
        sizeof(receiver_addr_in)
    );

    if (result < 0) {
        throw SocketIoError("socket send");
    }
}

void Socket::close()
{
    if (this->sockfd >= 0) {
        shutdown(this->sockfd, SHUT_RDWR);
        ::close(this->sockfd);
    }
}

ReliableSocket::Inner::Inner(
    Socket&& udp,
    uint64_t max_req_attempt,
    Channel<Enveloped>::Receiver&& handler_to_req_receiver
) :
    udp(std::move(udp)),
    max_req_attempt(max_req_attempt),
    handler_to_req_receiver(handler_to_req_receiver)
{
}

ReliableSocket::SentReq::SentReq(Channel<Enveloped>::Receiver&& channel) :
    channel(channel)
{
}

Enveloped ReliableSocket::SentReq::receive_resp() &&
{
    return std::move(*this).channel.receive();
}

ReliableSocket::ReceivedReq::ReceivedReq(
    std::shared_ptr<Inner> const& inner,
    Enveloped req_enveloped
) :
    inner(inner),
    req_enveloped_(req_enveloped)
{
}

Enveloped const& ReliableSocket::ReceivedReq::req_enveloped() const
{
    return this->req_enveloped_;
}

void ReliableSocket::ReceivedReq::send_resp(Message response) &&
{
    // TODO
}

ReliableSocket::ReliableSocket(
    std::shared_ptr<ReliableSocket::Inner> inner,
    Channel<Enveloped>&& input_to_handler_channel,
    Channel<Enveloped>::Sender&& handler_to_req_receiver
) :
    inner(inner),
    input_thread([] {
    }),
    handler_thread([] {
    }),
    bumper_thread([] {
    })
{
}

ReliableSocket::ReliableSocket(
    Socket&& udp,
    uint64_t max_req_attempt,
    Channel<Enveloped>&& input_to_handler_channel,
    Channel<Enveloped>&& handler_to_recv_req_channel
) :
    ReliableSocket(
        std::shared_ptr<Inner>(new Inner(
            std::move(udp),
            max_req_attempt,
            std::move(handler_to_recv_req_channel.receiver)
        )),
        std::move(input_to_handler_channel),
        std::move(handler_to_recv_req_channel.sender)
    )
{
}

ReliableSocket::ReliableSocket(Socket&& udp, uint64_t max_req_attempt) :
    ReliableSocket(
        std::move(udp),
        max_req_attempt,
        Channel<Enveloped>(),
        Channel<Enveloped>()
    )
{
}
