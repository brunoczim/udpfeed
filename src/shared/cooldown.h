#ifndef SHARED_COOLDOWN_H_
#define SHARED_COOLDOWN_H_ 1

#include <memory>

class Cooldown {
    public:
        class Config {
            public:
                virtual std::shared_ptr<Cooldown> start() const = 0;
                virtual ~Config();
        };

        virtual bool tick() = 0;
        virtual bool alive() const = 0;

        virtual ~Cooldown();
};

class BinaryExpCooldown : public Cooldown {
    public:
        class Config : public Cooldown::Config {
            public:
                uint64_t numer;
                uint64_t denom;
                uint64_t max_attempts;

                Config();

                virtual std::shared_ptr<Cooldown> start() const;
        };

    private:
        BinaryExpCooldown::Config config;
        uint64_t attempts;
        uint64_t counter;

        BinaryExpCooldown(Config const& config);

    public:
        virtual bool tick();
        virtual bool alive() const;
};

#endif
