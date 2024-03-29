#include "socket.h"
#include "log.h"
#include "time.h"
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

Socket::Socket(Address bind_addr, size_t max_message_size) :
    Socket(max_message_size)
{
    struct sockaddr_in native_bind_addr;

    native_bind_addr.sin_family = AF_INET;
    native_bind_addr.sin_port = htons(bind_addr.port);
    native_bind_addr.sin_addr.s_addr = htonl(bind_addr.ipv4);
    bzero(&native_bind_addr.sin_zero, 8);

    int status = bind(
        this->sockfd,
        (struct sockaddr *) &native_bind_addr,
        sizeof(native_bind_addr)
    );
    if (status < 0) {
        throw SocketIoError("socket bind");
    }
}

Socket::Socket(Socket&& other) :
    sockfd(other.sockfd),
    max_message_size(other.max_message_size)
{
    other.sockfd = -1;
}

Socket& Socket::operator=(Socket&& other)
{
    if (this->sockfd != other.sockfd) {
        this->close();
    }
    this->max_message_size = other.max_message_size;
    this->sockfd = other.sockfd;
    other.sockfd = -1;
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
    fds[0].revents = 0;

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
        ::close(this->sockfd);
    }
}

UnexpectedMessageStep::UnexpectedMessageStep(
    char const *expected,
    Enveloped enveloped
) :
    enveloped_(enveloped),
    message("expected ")
{
    this->message += expected;
    this->message += ", got message with tag = ";
    this->message += enveloped.message.body->tag().to_string();
    this->message += ", seqn = ";
    this->message += std::to_string(enveloped.message.header.seqn);
    this->message += ", timestamp = ";
    this->message += std::to_string(enveloped.message.header.timestamp);
    this->message += ", from ";
    this->message += enveloped.remote.to_string();
}

Enveloped UnexpectedMessageStep::enveloped() const
{
    return this->enveloped_;
}

char const *UnexpectedMessageStep::what() const noexcept
{
    return this->message.c_str();
}

UnexpectedMessageStep::~UnexpectedMessageStep()
{
}

ExpectedRequest::ExpectedRequest(Enveloped enveloped) :
    UnexpectedMessageStep("request", enveloped)
{
}

ExpectedResponse::ExpectedResponse(Enveloped enveloped) :
    UnexpectedMessageStep("response", enveloped)
{
}

MissedResponse::MissedResponse(Enveloped const& req_enveloped) :
    req_enveloped_(req_enveloped),
    message("missed a response from the request enveloped with tag = ")
{
    this->message += req_enveloped.message.body->tag().to_string();
    this->message += ", seqn = ";
    this->message += std::to_string(req_enveloped.message.header.seqn);
    this->message += ", timestamp = ";
    this->message += std::to_string(req_enveloped.message.header.timestamp);
    this->message += ", to ";
    this->message += req_enveloped.remote.to_string();
}

Enveloped const& MissedResponse::req_enveloped() const
{
    return this->req_enveloped_;
}

char const *MissedResponse::what() const noexcept
{
    return this->message.c_str();
}

ReliableSocket::PendingResponse::PendingResponse(
    Enveloped enveloped,
    uint64_t max_req_attempts,
    std::optional<Channel<Enveloped>::Sender>&& callback
) :
    request(enveloped),
    cooldown_attempt(0),
    cooldown_counter(1),
    remaining_attempts(max_req_attempts),
    callback(callback)
{
}

ReliableSocket::Connection::Connection() : Connection(Enveloped())
{
}

ReliableSocket::Connection::Connection(Enveloped connect_request) :
    remote_address(connect_request.remote),
    disconnecting(false),
    disconnect_counter(0)
{
}

ReliableSocket::Inner::Inner(
    Socket&& udp,
    Config const& config,
    Channel<Enveloped>::Receiver&& handler_to_req_receiver
) :
    udp(std::move(udp)),
    config(config),
    handler_to_req_receiver(std::move(handler_to_req_receiver)),
    election_counter(0)
{
}

ReliableSocket::Config const& ReliableSocket::Inner::used_config() const
{
    return this->config;
}

bool ReliableSocket::Inner::is_connected()
{
    try {
        return this->handler_to_req_receiver.is_connected();
    } catch (ChannelDisconnected const& exc) {
        return false;
    }
}

void ReliableSocket::Inner::send_req(
    Enveloped enveloped,
    std::optional<Channel<Enveloped>::Sender>&& callback
)
{
    if (enveloped.message.body->tag().step != MSG_REQ) {
        throw ExpectedRequest(enveloped);
    }

    std::unique_lock lock(this->net_control_mutex);

    this->unsafe_send_req(enveloped, std::move(callback));
}

void ReliableSocket::Inner::unsafe_send_req(
    Enveloped enveloped,
    std::optional<Channel<Enveloped>::Sender>&& callback
)
{
    enveloped.message.header.election_counter =
        this->unsafe_get_election_counter();
    enveloped.message.header.fill_req();

    if (this->connections.find(enveloped.remote) == this->connections.end()) {
        switch (enveloped.message.body->tag().type) {
            case MSG_CLIENT_CONN:
            case MSG_SERVER_CONN:
                this->connections.insert(std::make_pair(
                    enveloped.remote,
                    Connection(enveloped)
                ));
                break;

            case MSG_DISCONNECT: {
                Enveloped response;
                response.remote = enveloped.remote;
                response.message.header.election_counter =
                    this->unsafe_get_election_counter();
                response.message.header.fill_resp(
                    enveloped.message.header.seqn
                );
                response.message.body = std::shared_ptr<MessageBody>(
                    new MessageDisconnectResp
                );
                std::optional<Channel<Enveloped>::Sender> moved_callback
                    = std::move(callback);
                if (moved_callback) {
                    moved_callback->send(response);
                }
                return;
            }

            default: {
                auto moved_callback_ = std::move(callback);
                return;
            }
        }
    }

    Connection& connection = this->connections[enveloped.remote];

    bool was_disconnecting = false;
    if (enveloped.message.body->tag().type == MSG_DISCONNECT) {
        was_disconnecting = connection.disconnecting;
        connection.disconnecting = true;
    }

    if (!was_disconnecting || callback.has_value()) {
        connection.pending_responses.insert(std::make_pair(
            enveloped.message.header.seqn, 
            PendingResponse(
                enveloped,
                this->config.max_req_attempts,
                std::move(callback)
            )
        ));

        this->udp.send(enveloped);
    }
}

void ReliableSocket::Inner::send_resp(Enveloped enveloped)
{
    if (enveloped.message.body->tag().step != MSG_RESP) {
        throw ExpectedResponse(enveloped);
    }

    std::unique_lock lock(this->net_control_mutex);

    this->unsafe_send_resp(enveloped);
}

void ReliableSocket::Inner::unsafe_send_resp(Enveloped enveloped)
{
    Connection& connection = this->connections[enveloped.remote];

    if (
        connection.cached_sent_resp_queue.size()
        < this->config.max_cached_sent_resps
    ) {
        connection.cached_sent_resps.erase(
            connection.cached_sent_resp_queue.front()
        );
        connection.cached_sent_resp_queue.pop();
    }

    connection.cached_sent_resp_queue.push(enveloped.message.header.seqn);
    connection.cached_sent_resps.insert(std::make_pair(
        enveloped.message.header.seqn,
        enveloped
    ));
    this->udp.send(enveloped);
}

Enveloped ReliableSocket::Inner::unsafe_forceful_disconnect(Address remote)
{
    this->connections.erase(remote);
    Enveloped fake_req;
    fake_req.remote = remote;
    fake_req.message.body = std::shared_ptr<MessageBody>(
        new MessageDisconnectReq
    );
    fake_req.message.header.election_counter =
        this->unsafe_get_election_counter();
    fake_req.message.header.fill_req();
    return fake_req;
}

std::optional<Enveloped> ReliableSocket::Inner::receive_raw(int poll_timeout_ms)
{
    while (this->is_connected()) {
        try {
            if (auto enveloped = this->udp.receive(poll_timeout_ms)) {
                return std::optional<Enveloped>(enveloped);
            }
        } catch (MessageOutOfProtocol const &exc) {
        }
    }
    return std::optional<Enveloped>();
}

Enveloped ReliableSocket::Inner::receive()
{
    return this->handler_to_req_receiver.receive();
}

std::optional<Enveloped> ReliableSocket::Inner::handle(Enveloped enveloped)
{
    std::unique_lock lock(this->net_control_mutex);

    if (
        auto search = this->connections.find(enveloped.remote);
        search != this->connections.end()
    ) {
        Connection& connection = std::get<1>(*search);
        connection.disconnect_counter = 0;
    }

    switch (enveloped.message.body->tag().step) {
        case MSG_REQ:
            return this->unsafe_handle_req(enveloped);

        case MSG_RESP:
            this->unsafe_handle_resp(enveloped);
            break;
    }

    return std::optional<Enveloped>();
}

std::optional<Enveloped> ReliableSocket::Inner::unsafe_handle_req(
    Enveloped enveloped
)
{
    if (enveloped.message.body->tag().type == MSG_PING) {
        Enveloped response;
        response.remote = enveloped.remote;
        response.message.header.election_counter =
            this->unsafe_get_election_counter();
        response.message.header.fill_resp(
            enveloped.message.header.seqn
        );
        response.message.body = std::shared_ptr<MessageBody>(
            new MessagePingResp
        );
        this->udp.send(response);
        return std::optional<Enveloped>();
    }


    if (this->connections.find(enveloped.remote) == this->connections.end()) {
        switch (enveloped.message.body->tag().type) {
            case MSG_CLIENT_CONN:
            case MSG_SERVER_CONN:
                this->connections.insert(std::make_pair(
                    enveloped.remote,
                    Connection(enveloped)
                ));
                break;

            case MSG_DISCONNECT: {
                Enveloped response;
                response.remote = enveloped.remote;
                response.message.header.election_counter =
                    this->unsafe_get_election_counter();
                response.message.header.fill_resp(
                    enveloped.message.header.seqn
                );
                response.message.body = std::shared_ptr<MessageBody>(
                    new MessageDisconnectResp
                );
                this->udp.send(response);
                return std::optional<Enveloped>();
            }

            default: {
                Enveloped response;
                response.remote = enveloped.remote;
                response.message.header.election_counter =
                    this->unsafe_get_election_counter();
                response.message.header.fill_resp(
                    enveloped.message.header.seqn
                );
                response.message.body = std::shared_ptr<MessageBody>(
                    new MessageErrorResp(MSG_NO_CONNECTION)
                );
                this->udp.send(response);
                return std::optional<Enveloped>();
            }
        }
    }

    Connection& connection = this->connections[enveloped.remote];

     if (
        auto resp_search =
            connection.cached_sent_resps.find(enveloped.message.header.seqn);
        resp_search != connection.cached_sent_resps.end()
    ) {
        Enveloped response = std::get<1>(*resp_search);
        this->udp.send(response);
    } else if (
        connection.received_seqn_set.add(enveloped.message.header.seqn)
    ) {
        return std::make_optional(enveloped);
    }

    return std::optional<Enveloped>();
}

void ReliableSocket::Inner::unsafe_handle_resp(Enveloped enveloped)
{
    if (
        auto conn_search = this->connections.find(enveloped.remote);
        conn_search != this->connections.end()
    ) {
        Connection& connection = std::get<1>(*conn_search);
        if (auto pending_node = connection.pending_responses.extract(
            enveloped.message.header.seqn
        )) {
            PendingResponse pending = pending_node.mapped();
            if (pending.callback.has_value()) {
                pending.callback->send(enveloped);
            }
        }
    }
    if (enveloped.message.body->tag().type == MSG_DISCONNECT) {
        this->connections.erase(enveloped.remote);
    }
}

std::vector<Enveloped> ReliableSocket::Inner::bump()
{
    std::unique_lock lock(this->net_control_mutex);

    std::vector<Enveloped> fake_disconnect_reqs;

    std::set<uint64_t> seqn_to_be_removed;
    std::set<Address> addresses_to_be_removed;
    for (auto& conn_entry : this->connections) {
        Address address = std::get<0>(conn_entry);
        Connection& connection = std::get<1>(conn_entry);

        for (auto& pending_entry : connection.pending_responses) {
            uint64_t seqn = std::get<0>(pending_entry);
            PendingResponse& pending = std::get<1>(pending_entry);

            if (pending.cooldown_counter == 0) {
                if (pending.remaining_attempts == 0) {
                    seqn_to_be_removed.insert(seqn);
                    switch (pending.request.message.body->tag().type) {
                        case MSG_CLIENT_CONN:
                        case MSG_SERVER_CONN:
                        case MSG_DISCONNECT:
                            addresses_to_be_removed.insert(address);
                            break;
                    }
                } else {
                    pending.remaining_attempts--;
                    this->udp.send(pending.request);
                }

                pending.cooldown_attempt++;
                uint64_t exponent = pending.cooldown_attempt;
                exponent *= this->config.req_cooldown_numer;
                exponent /= this->config.req_cooldown_denom;
                pending.cooldown_counter = 1 << exponent;
            } else {
                pending.cooldown_counter--;
            }
        }

        for (uint64_t const& seqn : seqn_to_be_removed) {
            connection.pending_responses.erase(seqn);
        }
        seqn_to_be_removed.clear();

        connection.disconnect_counter++;

        if (connection.disconnect_counter > this->config.max_disconnect_count) {
            addresses_to_be_removed.insert(address);
        } else if (
            connection.disconnect_counter >= this->config.ping_start == 0
        ) {
            uint64_t offset =
                connection.disconnect_counter - this->config.ping_start;
            if (offset % this->config.ping_interval == 0) {
                Enveloped ping_request;
                ping_request.remote = address;
                ping_request.message.body = std::shared_ptr<MessageBody>(
                    new MessagePingReq
                );
                ping_request.message.header.election_counter = 
                    this->unsafe_get_election_counter();
                ping_request.message.header.fill_req();
                this->udp.send(ping_request);
            }
        }
    }

    for (Address address : addresses_to_be_removed) {
        fake_disconnect_reqs.push_back(
            this->unsafe_forceful_disconnect(address)
        );
    }

    return fake_disconnect_reqs;
}

void ReliableSocket::Inner::disconnect()
{
    this->handler_to_req_receiver.disconnect();

    std::unique_lock lock(this->net_control_mutex);

    Enveloped disconnect_req;
    disconnect_req.message.body = std::shared_ptr<MessageBody>(
        new MessageDisconnectReq
    );
    for (auto& conn_entry : this->connections) {
        Address address = std::get<0>(conn_entry);
        disconnect_req.remote = address;
        this->unsafe_send_req(
            disconnect_req,
            std::optional<Channel<Enveloped>::Sender>()
        );
    }
    this->connections.clear();
}

void ReliableSocket::Inner::unsafe_set_election_counter(uint64_t counter)
{
    this->election_counter = counter;
}

uint64_t ReliableSocket::Inner::unsafe_get_election_counter() const
{
    return this->election_counter;
}

void ReliableSocket::Inner::set_election_counter(uint64_t counter)
{
    std::unique_lock lock(this->net_control_mutex);
    this->unsafe_set_election_counter(counter);
}

uint64_t ReliableSocket::Inner::get_election_counter()
{
    std::unique_lock lock(this->net_control_mutex);
    return this->unsafe_get_election_counter();
}

ReliableSocket::Config::Config() :
    req_cooldown_numer(11),
    req_cooldown_denom(16),
    max_req_attempts(23),
    max_cached_sent_resps(100),
    bump_interval_nanos(250 * 1000),
    max_disconnect_count(5000),
    ping_start(1000),
    ping_interval(500),
    poll_timeout_ms(10)
{
}

ReliableSocket::Config& ReliableSocket::Config::with_req_cooldown_numer(
    uint64_t val
)
{
    this->req_cooldown_numer = val;
    return *this;
}

ReliableSocket::Config& ReliableSocket::Config::with_req_cooldown_denom(
    uint64_t val
)
{
    this->req_cooldown_denom = val;
    return *this;
}

ReliableSocket::Config& ReliableSocket::Config::with_max_req_attempts(
    uint64_t val
)
{
    this->max_req_attempts = val;
    return *this;
}

ReliableSocket::Config& ReliableSocket::Config::with_max_cached_sent_resps(
    uint64_t val
)
{
    this->max_cached_sent_resps = val;
    return *this;
}

ReliableSocket::Config& ReliableSocket::Config::with_bump_interval_nanos(
    uint64_t val
)
{
    this->bump_interval_nanos = val;
    return *this;
}

ReliableSocket::Config& ReliableSocket::Config::with_max_disconnect_count(
    uint64_t val
)
{
    this->max_disconnect_count = val;
    return *this;
}

ReliableSocket::Config& ReliableSocket::Config::with_ping_start(
    uint64_t val
)
{
    this->ping_start = val;
    return *this;
}

ReliableSocket::Config& ReliableSocket::Config::with_ping_interval(
    uint64_t val
)
{
    this->ping_interval = val;
    return *this;
}

ReliableSocket::Config& ReliableSocket::Config::with_poll_timeout_ms(int val)
{
    this->poll_timeout_ms = val;
    return *this;
}

uint64_t ReliableSocket::Config::min_response_timeout_ns() const
{
    uint64_t nanos = 0;

    for (uint64_t i = 0; i <= this->max_req_attempts; i++) {
        nanos += 1 << (i * this->req_cooldown_numer / this->req_cooldown_denom);
    }

    return nanos * this->bump_interval_nanos;
}

uint64_t ReliableSocket::Config::min_ping_timeout_ns() const
{
    return this->bump_interval_nanos * this->max_disconnect_count;
}

void ReliableSocket::Config::report(std::ostream& stream) const
{
    ReportTime min_response_timeout_ns(
        this->min_response_timeout_ns(),
        ReportTime::NS
    );
    ReportTime min_ping_timeout_ns(
        this->min_ping_timeout_ns(),
        ReportTime::NS
    );
    stream
        << "Using configuration with:"
        << std::endl
        << "- minimum timeout for responses: "
        << min_response_timeout_ns.to_string()
        << std::endl
        << "- minimum timeout before disconnecting: "
        << min_ping_timeout_ns.to_string()
        << std::endl
    ;
}

ReliableSocket::SentReq::SentReq(
    Enveloped req_enveloped,
    Channel<Enveloped>::Receiver&& channel
) :
    req_enveloped_(req_enveloped),
    channel(channel)
{
}

Enveloped const& ReliableSocket::SentReq::req_enveloped() const
{
    return this->req_enveloped_;
}

Enveloped ReliableSocket::SentReq::receive_resp() &&
{
    Enveloped req_enveloped = this->req_enveloped();
    try {
        return std::move(*this).channel.receive();
    } catch (ChannelDisconnected const& exc) {
        throw MissedResponse(req_enveloped);
    }
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

void ReliableSocket::ReceivedReq::send_resp(
    std::shared_ptr<MessageBody> const& response
) &&
{
    Enveloped enveloped_resp;
    enveloped_resp.remote = this->req_enveloped_.remote;
    enveloped_resp.message.header.election_counter =
        this->req_enveloped_.message.header.election_counter;
    enveloped_resp.message.header.fill_resp(
        this->req_enveloped_.message.header.seqn
    );
    enveloped_resp.message.body = response;
    this->inner->send_resp(enveloped_resp);
}

ReliableSocket::DisconnectGuard::DisconnectGuard(
    std::shared_ptr<ReliableSocket> const& socket
) :
    socket(socket)
{
}

ReliableSocket::DisconnectGuard::~DisconnectGuard()
{
    this->socket->disconnect();
}

ReliableSocket const& ReliableSocket::DisconnectGuard::operator*() const
{
    return *this->socket;
}

ReliableSocket& ReliableSocket::DisconnectGuard::operator*()
{
    return *this->socket;
}

ReliableSocket const* ReliableSocket::DisconnectGuard::operator->() const
{
    return this->socket.operator->();
}

ReliableSocket* ReliableSocket::DisconnectGuard::operator->()
{
    return this->socket.operator->();
}

ReliableSocket::ReliableSocket(
    std::shared_ptr<ReliableSocket::Inner> inner,
    Config const& config,
    Channel<Enveloped>&& input_to_handler_channel,
    Channel<Enveloped>::Sender&& handler_to_req_receiver
) :
    inner(inner),

    input_thread([
        inner,
        poll_timeout_ms = config.poll_timeout_ms,
        channel = std::move(input_to_handler_channel.sender)
    ] () mutable {
        try {
            bool connected = true;
            while (connected) {
                try {
                    if (auto enveloped = inner->receive_raw(poll_timeout_ms)) {
                        channel.send(*enveloped); 
                    } else {
                        connected = false;
                    }
                } catch (DeserializationError const& exc) {
                    Logger::with([&exc] (auto& output) {
                        output
                            << "failed to deserialize a packet: "
                            << exc.what()
                            << std::endl;
                    });
                }
            }
        } catch (ChannelDisconnected const& exc) {
        }
    }),

    handler_thread([
        inner,
        from_input = std::move(input_to_handler_channel.receiver),
        to_req_receiver = handler_to_req_receiver
    ] () mutable {
        try {
            for (;;) {
                Enveloped enveloped = from_input.receive();
                if (auto request = inner->handle(enveloped)) {
                    to_req_receiver.send(*request);
                }
            }
        } catch (ChannelDisconnected const& exc) {
        }
    }),

    bumper_thread([
        inner,
        bump_interval_nanos = config.bump_interval_nanos,
        channel = std::move(handler_to_req_receiver)
    ] () mutable {
        try {
            std::chrono::nanoseconds interval(bump_interval_nanos);
            std::chrono::nanoseconds actual_interval = interval;
            while (inner->is_connected()) {
                std::this_thread::sleep_for(actual_interval);
                std::chrono::time_point<std::chrono::system_clock> then =
                    std::chrono::system_clock::now();
                for (auto enveloped : inner->bump()) {
                    channel.send(enveloped);
                }
                std::chrono::time_point<std::chrono::system_clock> now =
                    std::chrono::system_clock::now();

                auto diff = now - then;

                if (interval > diff) {
                    actual_interval = interval - diff;
                } else {
                    actual_interval = std::chrono::nanoseconds(0);
                }
            }
        } catch (ChannelDisconnected const& exc) {
        }
    })
{
}

ReliableSocket::ReliableSocket(
    Socket&& udp,
    Config const& config,
    Channel<Enveloped>&& input_to_handler_channel,
    Channel<Enveloped>&& handler_to_recv_req_channel
) :
    ReliableSocket(
        std::shared_ptr<Inner>(new Inner(
            std::move(udp),
            config,
            std::move(handler_to_recv_req_channel.receiver)
        )),
        config,
        std::move(input_to_handler_channel),
        std::move(handler_to_recv_req_channel.sender)
    )
{
}

ReliableSocket::ReliableSocket(
    Socket&& udp,
    Config const& config
) :
    ReliableSocket(
        std::move(udp),
        config,
        Channel<Enveloped>(),
        Channel<Enveloped>()
    )
{
}

ReliableSocket::ReliableSocket(ReliableSocket&& other) :
    inner(std::move(other.inner)),

    input_thread(std::move(other.input_thread)),
    handler_thread(std::move(other.handler_thread)),
    bumper_thread(std::move(other.bumper_thread))
{
}

ReliableSocket& ReliableSocket::operator=(ReliableSocket&& other)
{
    this->close();
    this->inner = std::move(other.inner);
    this->input_thread = std::move(other.input_thread);
    this->handler_thread = std::move(other.handler_thread);
    this->bumper_thread = std::move(other.bumper_thread);

    return *this;
}

ReliableSocket::~ReliableSocket()
{
    this->close();
}

void ReliableSocket::close()
{
    if (this->inner) {
        this->inner->disconnect();
        this->input_thread.join();
        this->handler_thread.join();
        this->bumper_thread.join();
    }
}

ReliableSocket::Config const& ReliableSocket::config() const
{
    return this->inner->used_config();
}

ReliableSocket::SentReq ReliableSocket::send_req(Enveloped enveloped)
{
    Channel<Enveloped> channel;
    this->inner->send_req(
        enveloped,
        std::make_optional(std::move(channel.sender))
    );
    return ReliableSocket::SentReq(enveloped, std::move(channel.receiver));
}

ReliableSocket::ReceivedReq ReliableSocket::receive_req()
{
    Enveloped req_enveloped = this->inner->receive();
    return ReliableSocket::ReceivedReq(this->inner, req_enveloped);
}

void ReliableSocket::disconnect()
{
    this->inner->disconnect();
}

void ReliableSocket::disconnect_timeout(
    uint64_t interval_nanos,
    uint64_t intervals
)
{
    while (this->inner->is_connected() && intervals > 0) {
        std::this_thread::sleep_for(std::chrono::nanoseconds(interval_nanos));
        intervals--;
    }
    this->inner->disconnect();
}

uint64_t ReliableSocket::get_election_counter() const
{
    return this->inner->get_election_counter();
}

void ReliableSocket::set_election_counter(uint64_t counter)
{
    this->inner->set_election_counter(counter);
}
