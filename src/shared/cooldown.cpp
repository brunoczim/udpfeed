#include "cooldown.h"

BinaryExpCooldown::Config::Config() :
    numer(11),
    denom(16),
    max_attempts(23)
{
}

Cooldown::Config::~Config()
{
}

std::shared_ptr<Cooldown> BinaryExpCooldown::Config::start() const
{
    return std::shared_ptr<Cooldown>(new BinaryExpCooldown(*this));
}

Cooldown::~Cooldown()
{
}

BinaryExpCooldown::BinaryExpCooldown(Config const& config) :
    config(config),
    attempts(0),
    counter(1)
{
}

bool BinaryExpCooldown::tick()
{
    if (this->counter == 0) {
        this->attempts++;
        if (!this->alive()) {
            return false;
        }
        uint64_t exponent = this->attempts;
        exponent *= this->config.numer;
        exponent /= this->config.denom;
        this->counter = 1 << exponent;
    } else {
        this->counter--;
    }

    return true;
}

bool BinaryExpCooldown::alive() const
{
    return this->attempts > this->config.max_attempts;
}

