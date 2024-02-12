#ifndef SHARED_COOLDOWN_H_
#define SHARED_COOLDOWN_H_ 1

#include <memory>

enum CooldownTick {
    COOLDOWN_IDLE,
    COOLDOWN_CYCLED,
    COOLDOWN_DIED
};

class LinearCooldown {
    public:
        class Config {
            public:
                uint64_t ticks_per_attempt;
                uint64_t max_attempts;

                Config();

                LinearCooldown start() const;
        };

    private:
        LinearCooldown::Config config;
        uint64_t counter;

        LinearCooldown(Config const& config);

    public:
        CooldownTick tick();
};

class BinaryExpCooldown {
    public:
        class Config {
            public:
                uint64_t numer;
                uint64_t denom;
                uint64_t max_attempts;

                Config();

                BinaryExpCooldown start() const;
        };

    private:
        BinaryExpCooldown::Config config;
        uint64_t attempts;
        uint64_t counter;

        BinaryExpCooldown(Config const& config);

    public:
        CooldownTick tick();
};

#endif
