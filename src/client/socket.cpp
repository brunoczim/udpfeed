#include "socket.h"

ClientSocket::Config::Config() :
    bump_interval_nanos(100 * 1000),
    poll_timeout_ms(10)
{
}

bool ClientSocket::Inner::is_connected()
{
    try {
        return this->handler_to_req_receiver.is_connected();
    } catch (ChannelDisconnected const& exc) {
        return false;
    }
}

std::optional<Enveloped> ClientSocket::Inner::receive_raw()
{
    while (this->is_connected()) {
        try {
            if (
                auto enveloped = this->udp.receive(this->config.poll_timeout_ms)
            ) {
                return std::optional<Enveloped>(enveloped);
            }
        } catch (MessageOutOfProtocol const &exc) {
        }
    }
    return std::optional<Enveloped>();
}

std::optional<Enveloped> ClientSocket::Inner::handle(Enveloped enveloped)
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

std::vector<Enveloped> ClientSocket::Inner::bump()
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

ClientSocket(
    Socket&& udp,
    ClientSocket::Config const& config = ClientSocket::Config()
) :
    ClientSocket(
        std::move(udp),
        config,
        Channel<Enveloped>(),
        Channel<Enveloped>()
    )
{
}

ClientSocket::ClientSocket(
    Socket&& udp,
    ClientSocket::Config const& config,
    Channel<Enveloped>&& input_to_handler_channel,
    Channel<Enveloped>&& handler_to_req_channel
) :
    ClientSocket(
        new ClientSocket::Inner(
            std::move(udp),
            config,
            std::move(handler_to_req),
        ),
        std::move(input_to_handler_channel),
        std::move(handler_to_req_channel.receiver)
    )
{
}

ClientSocket::ClientSocket(
    std::shared_ptr<ClientSocket::Inner> const& inner,
    Channel<Enveloped>&& input_to_handler_channel,
    Channel<Enveloped>::Sender&& handler_to_req_receiver
) :
    inner(inner),

    input_thread([
        inner,
        channel = std::move(input_to_handler_channel.sender)
    ] () mutable {
        try {
            bool connected = true;
            while (connected) {
                try {
                    if (auto enveloped = inner->receive_raw()) {
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
        channel = std::move(handler_to_req_receiver)
    ] () mutable {
        try {
            std::chrono::nanoseconds interval(inner.config.bump_interval_nanos);
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
