#include "cooldown.h"

LinearCooldown::Config::Config() :
    ticks_per_attempt(500),
    max_attempts(5000)
{
}

LinearCooldown LinearCooldown::Config::start() const
{
    return LinearCooldown(*this);
}

LinearCooldown::LinearCooldown(Config const& config) :
    config(config),
    counter(config.ticks_per_attempt * (config.max_attempts + 1))
{
}

CooldownTick LinearCooldown::tick()
{
    if (this->counter == 0) {
        return COOLDOWN_DIED;
    }

    this->counter--;
    
    if (this->counter % this->config.ticks_per_attempt == 0) {
        return COOLDOWN_CYCLED;
    }

    return COOLDOWN_IDLE;
}

BinaryExpCooldown::Config::Config() :
    numer(11),
    denom(16),
    max_attempts(23)
{
}

BinaryExpCooldown BinaryExpCooldown::Config::start() const
{
    return BinaryExpCooldown(*this);
}

BinaryExpCooldown::BinaryExpCooldown(Config const& config) :
    config(config),
    attempts(0),
    counter(1)
{
}

CooldownTick BinaryExpCooldown::tick()
{
    if (this->counter == 0) {
        if (this->attempts >= this->config.max_attempts) {
            return COOLDOWN_DIED;
        }

        this->attempts++;
        uint64_t exponent = this->attempts;
        exponent *= this->config.numer;
        exponent /= this->config.denom;
        this->counter = 1 << exponent;
        return COOLDOWN_CYCLED;
    } 

    this->counter--;
    return COOLDOWN_IDLE;
}
