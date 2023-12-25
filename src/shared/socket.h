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
        Socket(Address bind_addr, size_t max_message_size);
        Socket(Socket&& other);
        Socket(Socket const& other) = delete;
        Socket& operator=(Socket const& obj) = delete;
        Socket& operator=(Socket&& obj);

        ~Socket();

        Enveloped receive();
        std::optional<Enveloped> receive(int timeout_ms);

        void send(Enveloped const& enveloped);
    
    private:
        void close();
};

class UnexpectedMessageStep : public std::exception {
    private:
        Enveloped enveloped_;
        std::string message;

    protected:
        UnexpectedMessageStep(char const *expected, Enveloped enveloped);

    public:
        Enveloped enveloped() const;

        virtual char const *what() const noexcept;
        
        virtual ~UnexpectedMessageStep();
};

class ExpectedRequest : public UnexpectedMessageStep {
    public:
        ExpectedRequest(Enveloped enveloped);
};

class ExpectedResponse : public UnexpectedMessageStep {
    public:
        ExpectedResponse(Enveloped enveloped);
};

class MissedResponse : public std::exception {
    private:
        Enveloped req_enveloped_;
        std::string message;

    public:
        MissedResponse(Enveloped const& req_enveloped);

        Enveloped const& req_enveloped() const;

        virtual char const *what() const noexcept;
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
                Enveloped request;
                uint64_t cooldown_attempt;
                uint64_t cooldown_counter;
                uint64_t remaining_attempts;
                std::optional<Channel<Enveloped>::Sender> callback;

                PendingResponse(
                    Enveloped enveloped,
                    uint64_t max_req_attempts,
                    std::optional<Channel<Enveloped>::Sender>&& callback
                );
        };

        class Connection {
            public:
                Address remote_address;
                bool disconnecting;
                uint64_t max_req_attemtps;
                uint64_t max_cached_sent_resps;
                uint64_t min_accepted_req_seqn;
                std::queue<uint64_t> cached_sent_resp_queue;
                std::map<uint64_t, Enveloped> cached_sent_resps;
                std::map<uint64_t, PendingResponse> pending_responses;

                Connection();

                Connection(
                    uint64_t max_req_attemtps,
                    uint64_t max_cached_sent_resps,
                    Enveloped connect_request
                );
        };

        class Inner {
            private:
                Socket udp;
                uint64_t max_req_attempts;
                uint64_t max_cached_sent_resps;

                Channel<Enveloped>::Receiver handler_to_req_receiver;

                std::mutex net_control_mutex;
                std::map<Address, Connection> connections;

            public:
                Inner(
                    Socket&& udp,
                    uint64_t max_req_attempts,
                    uint64_t max_cached_sent_resps,
                    Channel<Enveloped>::Receiver&& handler_to_req_receiver
                );

                bool is_connected();

                void send_req(
                    Enveloped enveloped,
                    std::optional<Channel<Enveloped>::Sender>&& callback
                );

                void unsafe_send_req(
                    Enveloped enveloped,
                    std::optional<Channel<Enveloped>::Sender>&& callback
                );

                void send_resp(Enveloped enveloped);

                void unsafe_send_resp(Enveloped enveloped);

                std::optional<Enveloped> receive_raw(int poll_timeout_ms);

                Enveloped receive();

                std::optional<Enveloped> handle(Enveloped enveloped);
                
                std::optional<Enveloped> unsafe_handle_req(Enveloped enveloped);

                void unsafe_handle_resp(Enveloped enveloped);

                void bump();

                void disconnect();
        };

    public:
        class Config {
            public:
                uint64_t max_req_attempts;
                uint64_t max_cached_sent_resps;
                uint64_t bump_interval_nanos;
                int poll_timeout_ms;

                Config();

                Config& with_max_req_attempts(uint64_t val);
                Config& with_max_cached_sent_resps(uint64_t val);
                Config& with_bump_interval_nanos(uint64_t val);
                Config& with_poll_timeout_ms(int val);
        };

        class SentReq {
            private:
                friend ReliableSocket;

                Enveloped req_enveloped_;

                Channel<Enveloped>::Receiver channel;

                SentReq(
                    Enveloped req_enveloped,
                    Channel<Enveloped>::Receiver&& channel
                );
            public:
                Enveloped const& req_enveloped() const;

                Enveloped receive_resp() &&;
        };

        class ReceivedReq {
            private:
                friend ReliableSocket;

                std::shared_ptr<Inner> inner;
                Enveloped req_enveloped_;

                ReceivedReq(
                    std::shared_ptr<Inner> const& inner,
                    Enveloped req_enveloped
                );
            public:
                Enveloped const& req_enveloped() const;

                void send_resp(std::shared_ptr<MessageBody> const& response) &&;
        };

        class DisconnectGuard {
            private:
                std::shared_ptr<ReliableSocket> socket;
            public:
                DisconnectGuard(std::shared_ptr<ReliableSocket> const& socket);
                ~DisconnectGuard();

                ReliableSocket const& operator*() const;
                ReliableSocket& operator*();

                ReliableSocket const* operator->() const;
                ReliableSocket* operator->();
        };

    private:
        std::shared_ptr<Inner> inner;
        std::thread input_thread;
        std::thread handler_thread;
        std::thread bumper_thread;

        ReliableSocket(
            std::shared_ptr<ReliableSocket::Inner> inner,
            uint64_t bump_interval_nanos,
            int poll_timeout_ms,
            Channel<Enveloped>&& input_to_handler_channel,
            Channel<Enveloped>::Sender&& handler_to_req_receiver
        );

        ReliableSocket(
            Socket&& udp,
            Config config,
            Channel<Enveloped>&& input_to_handler_channel,
            Channel<Enveloped>&& handler_to_recv_req_channel
        );

    private:
        void close();

    public:
        ReliableSocket(
            Socket&& udp,
            Config config = Config()
        );

        ReliableSocket(ReliableSocket&& other);
        ReliableSocket& operator=(ReliableSocket&& other);

        ~ReliableSocket();

        SentReq send_req(Enveloped message);
        ReceivedReq receive_req();

        void disconnect();
};

#endif
