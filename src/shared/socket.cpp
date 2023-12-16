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
#include <chrono>

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

UnexpectedMessageStep::UnexpectedMessageStep(
    char const *expected,
    MessageHeader header,
    MessageTag tag
) :
    header_(header),
    tag_(tag),
    message("expected ")
{
    this->message += expected;
    this->message += ", got message with tag = ";
    this->message += tag.to_string();
    this->message += ", seqn = ";
    this->message += std::to_string(header.seqn);
    this->message += ", timestamp = ";
    this->message += std::to_string(header.timestamp);
}

MessageHeader UnexpectedMessageStep::header() const
{
    return this->header_;
}

MessageTag UnexpectedMessageStep::tag() const
{
    return this->tag_;
}

char const *UnexpectedMessageStep::what() const noexcept
{
    return this->message.c_str();
}

UnexpectedMessageStep::~UnexpectedMessageStep()
{
}

ExpectedRequest::ExpectedRequest(MessageHeader header, MessageTag tag) :
    UnexpectedMessageStep("request", header, tag)
{
}

ExpectedResponse::ExpectedResponse(MessageHeader header, MessageTag tag) :
    UnexpectedMessageStep("response", header, tag)
{
}

ReliableSocket::PendingResponse::PendingResponse(
    Enveloped enveloped,
    uint64_t max_req_attempts,
    Channel<Enveloped>::Sender&& callback
) :
    request(enveloped),
    remaining_attempts(max_req_attempts),
    callback(callback)
{
}

ReliableSocket::Connection::Connection() : Connection(10, 20, Enveloped())
{
}

ReliableSocket::Connection::Connection(
    uint64_t max_req_attemtps,
    uint64_t max_cached_responses,
    Enveloped connect_request
) :
    max_req_attemtps(max_req_attemtps),
    max_cached_responses(max_cached_responses),
    min_accepted_req_seqn(0),
    estabilished(false),
    remote_address(connect_request.remote)
{
}

ReliableSocket::Inner::Inner(
    Socket&& udp,
    uint64_t max_req_attempts,
    uint64_t max_cached_responses,
    Channel<Enveloped>::Receiver&& handler_to_req_receiver
) :
    udp(std::move(udp)),
    max_req_attempts(max_req_attempts),
    max_cached_responses(max_cached_responses),
    handler_to_req_receiver(handler_to_req_receiver)
{
}

bool ReliableSocket::Inner::is_connected()
{
    return this->handler_to_req_receiver.is_connected();
}

void ReliableSocket::Inner::send_req(
    Enveloped enveloped,
    Channel<Enveloped>::Sender&& callback
)
{
    if (enveloped.message.body->tag().step != MSG_REQ) {
        throw ExpectedRequest(
            enveloped.message.header,
            enveloped.message.body->tag()
        );
    }

    std::unique_lock lock(this->net_control_mutex);

    if (enveloped.message.body->tag().type == MSG_CONNECT) { 
        this->connections.insert(std::make_pair(enveloped.remote, Connection(
            this->max_req_attempts,
            this->max_cached_responses,
            enveloped
        )));
    }

    Connection& connection = this->connections[enveloped.remote];

    connection.pending_responses.insert(std::make_pair(
        enveloped.message.header.seqn, 
        PendingResponse(enveloped, this->max_req_attempts, std::move(callback))
    ));

    this->udp.send(enveloped);
}

void ReliableSocket::Inner::send_resp(Enveloped enveloped)
{
    if (enveloped.message.body->tag().step != MSG_RESP) {
        throw ExpectedResponse(
            enveloped.message.header,
            enveloped.message.body->tag()
        );
    }

    std::unique_lock lock(this->net_control_mutex);

    Connection& connection = this->connections[enveloped.remote];
    if (
        connection.cached_response_queue.size()
        < connection.max_cached_responses
    ) {
        connection.cached_responses.erase(
            connection.cached_response_queue.front()
        );
        connection.cached_response_queue.pop();
    }

    connection.cached_response_queue.push(enveloped.message.header.seqn);
    connection.cached_responses.insert(std::make_pair(
        enveloped.message.header.seqn,
        enveloped
    ));
    this->udp.send(enveloped);
}

Enveloped ReliableSocket::Inner::receive_raw()
{
    return this->udp.receive();
}

Enveloped ReliableSocket::Inner::receive()
{
    return this->handler_to_req_receiver.receive();
}

void ReliableSocket::Inner::handle(Enveloped enveloped)
{
    std::unique_lock lock(this->net_control_mutex);
}

void ReliableSocket::Inner::bump()
{
    std::unique_lock lock(this->net_control_mutex);
    std::set<uint64_t> seqn_to_be_removed;
    std::set<Address> addresses_to_be_removed;
    for (auto& conn_entry : this->connections) {
        Address const& address = std::get<0>(conn_entry);
        Connection& connection = std::get<1>(conn_entry);
        for (auto& pending_entry : connection.pending_responses) {
            uint64_t seqn = std::get<0>(pending_entry);
            PendingResponse& pending = std::get<1>(pending_entry);
            if (pending.remaining_attempts == 0) {
                seqn_to_be_removed.insert(seqn);
                if (
                    pending.request.message.body->tag().type == MSG_DISCONNECT
                    || pending.request.message.body->tag().type == MSG_CONNECT
                ) {
                    addresses_to_be_removed.insert(address);
                }
            } else {
                pending.remaining_attempts--;
                this->udp.send(pending.request);
            }
        }
        for (uint64_t const& seqn : seqn_to_be_removed) {
            connection.pending_responses.erase(seqn);
        }
        seqn_to_be_removed.clear();
    }

    for (Address const& address : addresses_to_be_removed) {
        this->connections.erase(address);
    }
}

ReliableSocket::Config::Config() :
    max_req_attempts(10),
    max_cached_responses(25),
    bump_interval_nanos(500 * 100)
{
}

ReliableSocket::Config& ReliableSocket::Config::with_max_req_attempts(
    uint64_t val
)
{
    this->max_req_attempts = val;
    return *this;
}

ReliableSocket::Config& ReliableSocket::Config::with_max_cached_responses(
    uint64_t val
)
{
    this->max_cached_responses = val;
    return *this;
}

ReliableSocket::Config& ReliableSocket::Config::with_bump_interval_nanos(
    uint64_t val
)
{
    this->bump_interval_nanos = val;
    return *this;
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
    Enveloped envloped_resp;
    envloped_resp.remote = this->req_enveloped_.remote;
    envloped_resp.message = response;
    this->inner->send_resp(envloped_resp);
}

ReliableSocket::ReliableSocket(
    std::shared_ptr<ReliableSocket::Inner> inner,
    uint64_t bump_interval_nanos,
    Channel<Enveloped>&& input_to_handler_channel,
    Channel<Enveloped>::Sender&& handler_to_req_receiver
) :
    inner(inner),

    input_thread([
        inner,
        channel = std::move(input_to_handler_channel.sender)
    ] () mutable {
        try {
            for (;;) {
                Enveloped enveloped = inner->receive();
                channel.send(enveloped);
            }
        } catch (ReceiversDisconnected const& exc) {
        }
    }),

    handler_thread([
        inner,
        channel = std::move(input_to_handler_channel.receiver)
    ] () mutable {
        try {
            for (;;) {
                Enveloped enveloped = channel.receive();
                inner->handle(enveloped);
            }
        } catch (SendersDisconnected const& exc) {
        }
    }),

    bumper_thread([inner, bump_interval_nanos] () mutable {
        std::chrono::nanoseconds interval(bump_interval_nanos);
        while (inner->is_connected()) {
            std::this_thread::sleep_for(interval);
            inner->bump();
        }
    })
{
}

ReliableSocket::ReliableSocket(
    Socket&& udp,
    Config config,
    Channel<Enveloped>&& input_to_handler_channel,
    Channel<Enveloped>&& handler_to_recv_req_channel
) :
    ReliableSocket(
        std::shared_ptr<Inner>(new Inner(
            std::move(udp),
            config.max_req_attempts,
            config.max_cached_responses,
            std::move(handler_to_recv_req_channel.receiver)
        )),
        config.bump_interval_nanos,
        std::move(input_to_handler_channel),
        std::move(handler_to_recv_req_channel.sender)
    )
{
}

ReliableSocket::ReliableSocket(
    Socket&& udp,
    Config config
) :
    ReliableSocket(
        std::move(udp),
        config,
        Channel<Enveloped>(),
        Channel<Enveloped>()
    )
{
}

ReliableSocket::~ReliableSocket()
{
    {
        auto closed_ = std::move(this->inner);
    }
    this->input_thread.join();
    this->handler_thread.join();
    this->bumper_thread.join();
}

ReliableSocket::SentReq ReliableSocket::send_req(Enveloped enveloped)
{
    Channel<Enveloped> channel;
    this->inner->send_req(enveloped, std::move(channel.sender));
    return ReliableSocket::SentReq(std::move(channel.receiver));
}

ReliableSocket::ReceivedReq ReliableSocket::receive_req()
{
    Enveloped req_enveloped = this->inner->receive();
    return ReliableSocket::ReceivedReq(this->inner, req_enveloped);
}
