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

Message Socket::receive(Address &sender_addr_out)
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
    sender_addr_out.ipv4 = ntohl(sender_addr.sin_addr.s_addr);
    sender_addr_out.port = ntohs(sender_addr.sin_port);

    buf.resize(count);

    std::istringstream istream(buf);
    PlaintextDeserializer deserializer_impl(istream);
    Deserializer& deserializer = deserializer_impl;
    Message message;
    deserializer >> message;
    return message;
}

std::optional<Message> Socket::receive(int timeout_ms, Address &sender_addr_out)
{
    struct pollfd fds[1];
    fds[0].fd = this->sockfd;
    fds[0].events = POLLIN;

    int status = poll(fds, sizeof(fds) / sizeof(fds[0]), timeout_ms);
    if (status < 0) {
        throw SocketIoError("socket poll");
    }
    if (fds[0].revents & POLLIN) {
        return this->receive(sender_addr_out);
    }
    return std::optional<Message>();
}

void Socket::send(Message const& message, Address const& receiver_addr)
{
    std::ostringstream ostream;
    PlaintextSerializer serializer_impl(ostream);
    Serializer& serializer = serializer_impl;
    serializer << message;
    std::string buf = ostream.str();
    
    struct sockaddr_in receiver_addr_in;

    receiver_addr_in.sin_family = AF_INET;
    receiver_addr_in.sin_port = htons(receiver_addr.port);
    receiver_addr_in.sin_addr.s_addr = htonl(receiver_addr.ipv4);
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

ReliableSocket::ReqSent::ReqSent(Channel<Message>::Receiver&& channel) :
    channel(channel)
{
}

Message ReliableSocket::ReqSent::receive_response() &&
{
    return std::move(*this).channel.receive();
}

ReliableSocket::ReceivedReq::ReceivedReq(
    Channel<Message>::Sender&& channel,
    Message req_message
) :
    channel(channel),
    message(req_message)
{
}

Message const& ReliableSocket::ReceivedReq::req_message() const
{
    return this->message;
}

void ReliableSocket::ReceivedReq::send_response(Message response) &&
{
    std::move(*this).channel.send(response);
}

ReliableSocket::Inner::Inner(Socket&& udp, uint64_t max_req_attempt) :
    udp(std::move(udp)),
    max_req_attempt(max_req_attempt)
{
}

ReliableSocket::ReliableSocket(
    Channel<std::pair<Message, Channel<Message>::Sender>>&&
        send_channel,

    Channel<std::pair<Message, Channel<Message>::Sender>>&&
        receive_channel,

    std::shared_ptr<Inner> inner
) :
    send_thread([
        channel = std::move(send_channel.receiver),
        inner = inner
    ] {
    }),
    receive_thread([
        channel = std::move(receive_channel.sender),
        inner = inner
    ] {
    }),
    send_channel(std::move(send_channel.sender)),
    receive_channel(std::move(receive_channel.receiver))
{
}

ReliableSocket::ReliableSocket(Socket&& udp, uint64_t max_req_attempt) :
    ReliableSocket(
        Channel<std::pair<Message, Channel<Message>::Sender>>(),
        Channel<std::pair<Message, Channel<Message>::Sender>>(),
        std::move(std::shared_ptr<ReliableSocket::Inner>(
            new ReliableSocket::Inner(std::move(udp), max_req_attempt)
        ))
    )
{
}

ReliableSocket::ReqSent ReliableSocket::send_req(Message message)
{
    Channel<Message> channel;
    this->send_channel.send(std::make_pair(
        message, std::move(channel.sender)
    ));
    ReliableSocket::ReqSent sent_req(std::move(channel.receiver));
    return sent_req;
}

ReliableSocket::ReceivedReq ReliableSocket::receive_req()
{

    auto req_pair = this->receive_channel.receive();
    Message message = std::move(std::get<0>(req_pair));
    Channel<Message>::Sender channel = std::move(std::get<1>(req_pair));
    return ReliableSocket::ReceivedReq(std::move(channel), message);
}

ReliableSocket::~ReliableSocket()
{
    {
        auto send_channel = std::move(this->send_channel);
        auto recv_channel = std::move(this->receive_channel);
    }
    this->send_thread.join();
    this->receive_thread.join();
}
