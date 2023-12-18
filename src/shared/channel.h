#ifndef SHARED_CHANNEL_H_
#define SHARED_CHANNEL_H_ 1

#include <memory>
#include <queue>
#include <optional>
#include <exception>
#include <mutex>
#include <condition_variable>

class ChannelDisconnected : public std::exception {
    public:
        virtual char const *what() const noexcept = 0;
};

class SendersDisconnected : public ChannelDisconnected {
    public:
        virtual char const *what() const noexcept;
};

class ReceiversDisconnected : public ChannelDisconnected {
    public:
        virtual char const *what() const noexcept;
};

class UsageOfMovedChannel : public ChannelDisconnected {
    public:
        virtual char const *what() const noexcept;
};

template <typename T>
class Channel {
    private:
        class Inner {
            private:
                std::mutex mutex;
                uint64_t senders;
                uint64_t receivers;
                std::condition_variable cond_var;
                std::queue<T> messages;

            public:
                Inner();

                void sender_connected();
                void sender_disconnected();

                void receiver_connected();
                void receiver_disconnected();

                bool is_connected();

                void send(T message);

                std::optional<T> unsafe_try_receive();

                std::optional<T> try_receive();

                T receive();
        };

    public:
        class Sender {
            private:
                std::shared_ptr<Inner> inner;

                friend Channel;

                Sender(std::shared_ptr<Inner> const& inner);

            public:
                Sender(Sender const& other);
                Sender(Sender&& other);
                Sender& operator=(Sender const& other);
                Sender& operator=(Sender&& other);

                ~Sender();

                bool is_connected();

                void send(T message);

            private:
                void connected();
                void disconnected();
        };

        class Receiver {
            private:
                std::shared_ptr<Inner> inner;

                friend Channel;

                Receiver(std::shared_ptr<Inner> const& inner);

            public:
                Receiver(Receiver const& other);
                Receiver(Receiver&& other);
                Receiver& operator=(Receiver const& other);
                Receiver& operator=(Receiver&& other);

                ~Receiver();

                bool is_connected();

                std::optional<T> try_receive();

                T receive();

            private:
                void connected();
                void disconnected();
        };

    public:
        Channel();

        Sender sender;
        Receiver receiver;
};

template <typename T>
Channel<T>::Inner::Inner() : senders(0), receivers(0)
{
}

template <typename T>
void Channel<T>::Inner::sender_connected()
{
    std::unique_lock lock(this->mutex);
    this->senders++;
}

template <typename T>
void Channel<T>::Inner::sender_disconnected()
{
    std::unique_lock lock(this->mutex);
    this->senders--;
    if (this->senders == 0) {
        this->cond_var.notify_all();
    }
}

template <typename T>
void Channel<T>::Inner::receiver_connected()
{
    std::unique_lock lock(this->mutex);
    this->receivers++;
}

template <typename T>
void Channel<T>::Inner::receiver_disconnected()
{
    std::unique_lock lock(this->mutex);
    this->receivers--;
}

template <typename T>
bool Channel<T>::Inner::is_connected()
{
    std::unique_lock lock(this->mutex);
    return this->senders > 0 && this->receivers > 0;
}

template <typename T>
void Channel<T>::Inner::send(T message)
{
    std::unique_lock lock(this->mutex);
    if (this->receivers == 0) {
        throw ReceiversDisconnected();
    }
    this->messages.push(std::move(message));
    this->cond_var.notify_one();
}

template <typename T>
std::optional<T> Channel<T>::Inner::unsafe_try_receive()
{
    if (!this->messages.empty()) {
        T message = std::move(this->messages.front());
        this->messages.pop();
        return std::make_optional<T>(std::move(message));
    }
    if (this->senders == 0) {
        throw SendersDisconnected();
    }
    return std::nullopt;
}

template <typename T>
std::optional<T> Channel<T>::Inner::try_receive()
{
    std::unique_lock lock(this->mutex);
    return this->unsafe_try_receive();
}

template <typename T>
T Channel<T>::Inner::receive()
{
    std::unique_lock lock(this->mutex);
    for (;;) {
        if (auto message = this->unsafe_try_receive()) {
            return std::move(*message);
        }
        this->cond_var.wait(lock);
    }
}

template <typename T>
Channel<T>::Sender::Sender(std::shared_ptr<Inner> const& inner) : inner(inner)
{
    this->connected();
}

template <typename T>
Channel<T>::Sender::Sender(Sender const& other): inner(other.inner)
{
    this->connected();
}

template <typename T>
Channel<T>::Sender::Sender(Sender&& other): inner(std::move(other.inner))
{
}

template <typename T>
typename Channel<T>::Sender& Channel<T>::Sender::operator=(Sender const& other)
{
    this->disconnected();
    this->inner = other.inner;
    this->connected();
    return *this;
}

template <typename T>
typename Channel<T>::Sender& Channel<T>::Sender::operator=(Sender&& other)
{
    std::shared_ptr<Inner> temp_inner = std::move(other.inner);
    if (this->inner.get() != temp_inner.get()) {
        this->disconnected();
    }
    this->inner = std::move(temp_inner);
    return *this;
}

template <typename T>
Channel<T>::Sender::~Sender()
{
    this->disconnected();
}

template <typename T>
bool Channel<T>::Sender::is_connected()
{
    if (!this->inner) {
        throw UsageOfMovedChannel();
    }
    return this->inner->is_connected();
}

template <typename T>
void Channel<T>::Sender::send(T message)
{
    if (!this->inner) {
        throw UsageOfMovedChannel();
    }
    this->inner->send(std::move(message));
}

template <typename T>
void Channel<T>::Sender::connected()
{
    if (this->inner) {
        this->inner->sender_connected();
    }
}

template <typename T>
void Channel<T>::Sender::disconnected()
{
    if (this->inner) {
        this->inner->sender_disconnected();
    }
}

template <typename T>
Channel<T>::Receiver::Receiver(std::shared_ptr<Inner> const& inner) :
    inner(inner)
{
    this->connected();
}

template <typename T>
Channel<T>::Receiver::Receiver(Receiver const& other): inner(other.inner)
{
    this->connected();
}

template <typename T>
Channel<T>::Receiver::Receiver(Receiver&& other): inner(std::move(other.inner))
{
}

template <typename T>
typename Channel<T>::Receiver& Channel<T>::Receiver::operator=(
    Receiver const& other
)
{
    this->disconnected();
    this->inner = other.inner;
    this->connected();
    return *this;
}

template <typename T>
typename Channel<T>::Receiver& Channel<T>::Receiver::operator=(Receiver&& other)
{
    std::shared_ptr<Inner> temp_inner = std::move(other.inner);
    if (this->inner.get() != temp_inner.get()) {
        this->disconnected();
    }
    this->inner = std::move(temp_inner);
    return *this;
}

template <typename T>
Channel<T>::Receiver::~Receiver()
{
    this->disconnected();
}

template <typename T>
bool Channel<T>::Receiver::is_connected()
{
    if (!this->inner) {
        throw UsageOfMovedChannel();
    }
    return this->inner->is_connected();
}

template <typename T>
std::optional<T> Channel<T>::Receiver::try_receive()
{
    if (!this->inner) {
        throw UsageOfMovedChannel();
    }
    return this->inner->try_receive();
}

template <typename T>
T Channel<T>::Receiver::receive()
{
    if (!this->inner) {
        throw UsageOfMovedChannel();
    }
    return this->inner->receive();
}

template <typename T>
void Channel<T>::Receiver::connected()
{
    if (this->inner) {
        this->inner->receiver_connected();
    }
}

template <typename T>
void Channel<T>::Receiver::disconnected()
{
    if (this->inner) {
        this->inner->receiver_disconnected();
    }
}

template <typename T>
Channel<T>::Channel() :
    sender(std::shared_ptr<Inner>(new Inner())),
    receiver(sender.inner)
{
}

#endif
