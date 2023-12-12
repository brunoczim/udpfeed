#include "channel.h"

char const *SendersDisconnected::what() const noexcept
{
    return "all channel senders disconnected";
}

char const *ReceiversDisconnected::what() const noexcept
{
    return "all channel receivers disconnected";
}

char const *UsageOfMovedChannel::what() const noexcept
{
    return "usage of moved channel object";
}
