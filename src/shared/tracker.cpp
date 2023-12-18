#include "tracker.h"

char const *ThreadsExhausted::what() const noexcept
{
    return "thread number exhausted in thread tracker";
}

ThreadTracker::Inner::Inner(Channel<std::thread>::Sender&& joiner_sender) :
    thread_counter(0),
    joiner_sender(joiner_sender)
{
}

std::unique_lock<std::mutex> ThreadTracker::Inner::lock()
{
    return std::move(std::unique_lock(this->control_mutex));
}

uint64_t ThreadTracker::Inner::unsafe_next_id()
{
    if (this->thread_counter == UINT64_MAX) {
        throw ThreadsExhausted();
    }
    this->thread_counter++;
    return this->thread_counter;
}

void ThreadTracker::Inner::unsafe_start(uint64_t id, std::thread&& thread)
{
    this->threads.insert(std::make_pair(id, std::move(thread)));
}

void ThreadTracker::Inner::unsafe_finish(uint64_t id)
{
    if (auto node = this->threads.extract(id)) {
        try {
            this->joiner_sender.send(std::move(node.mapped()));
        } catch (UsageOfMovedChannel const& exc) {
        }
    }
}

void ThreadTracker::Inner::unsafe_finish_all()
{
    for (auto& node : this->threads) {
        this->joiner_sender.send(std::move(std::get<1>(node)));
    }
}

void ThreadTracker::Inner::close()
{
    auto closed_ = std::move(this->joiner_sender);
}

ThreadTracker::ThreadGuard::ThreadGuard(
    uint64_t id,
    std::shared_ptr<Inner> const& inner
) :
    id(id),
    inner(inner)
{
}

ThreadTracker::ThreadGuard::~ThreadGuard()
{
    std::unique_lock lock = std::move(this->inner->lock());
    this->inner->unsafe_finish(this->id);
}

ThreadTracker::ThreadTracker(Channel<std::thread>&& channel) :
    inner(new Inner(std::move(channel.sender))),
    joiner_thread([receiver = std::move(channel.receiver)] () mutable {
        try {
            for (;;) {
                std::thread thread = receiver.receive();
                if (thread.joinable()) {
                    thread.join();
                }
            }
        } catch (ChannelDisconnected const& exc) {
        }
    })
{
}

ThreadTracker::ThreadTracker() : ThreadTracker(Channel<std::thread>())
{
}

ThreadTracker::~ThreadTracker()
{
    {
        std::unique_lock lock(std::move(this->inner->lock()));
        this->inner->unsafe_finish_all();
        this->inner->close();
    }
    this->joiner_thread.join();
}
