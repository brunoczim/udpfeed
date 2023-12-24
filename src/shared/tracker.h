#ifndef SHARED_TRACKER_H_
#define SHARED_TRACKER_H_ 1

#include <exception>
#include <memory>
#include <map>
#include <thread>
#include <mutex>
#include "channel.h"

class ThreadsExhausted : public std::exception {
    public:
        virtual char const *what() const noexcept;
};

class ThreadTracker {
    private:
        class Inner {
            private:
                std::mutex control_mutex;
                uint64_t thread_counter;
                std::map<uint64_t, std::thread> threads;

                Channel<std::thread>::Sender joiner_sender;

            public:
                Inner(Channel<std::thread>::Sender&& joiner_sender);

                std::unique_lock<std::mutex> lock();

                uint64_t unsafe_next_id();
                void unsafe_start(uint64_t id, std::thread&& thread);
                void unsafe_finish(uint64_t id);
                void unsafe_finish_all();

                void close();
        };

        class ThreadGuard {
            private:
                uint64_t id;
                std::shared_ptr<Inner> inner;
            public:
                ThreadGuard(uint64_t id, std::shared_ptr<Inner> const& inner);
                ~ThreadGuard();
        };
        
        std::shared_ptr<Inner> inner;
        std::thread joiner_thread;

        ThreadTracker(Channel<std::thread>&& channel);

    public:
        ThreadTracker();
        ~ThreadTracker();

        template <typename F>
        void spawn(F&& task);

        void join_all();
};


template <typename F>
void ThreadTracker::spawn(F&& task)
{
    std::unique_lock<std::mutex> lock(std::move(this->inner->lock()));

    uint64_t thread_id = this->inner->unsafe_next_id();

    this->inner->unsafe_start(thread_id, std::move(std::thread([
        task = std::move(task),
        thread_id,
        inner = this->inner
    ] () mutable {
        ThreadGuard guard_(thread_id, inner);
        task();
    })));
}

#endif
