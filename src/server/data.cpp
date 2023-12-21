#include "data.h"

ServerProfileTable::Notification::Notification() :
    id(0),
    timestamp(0),
    pending_count(0)
{
}

void ServerProfileTable::Notification::serialize(Serializer& stream) const
{
    stream
        << this->id
        << this->timestamp
        << this->message
        << this->pending_count;
}

void ServerProfileTable::Notification::deserialize(Deserializer& stream)
{
    stream
        >> this->id
        >> this->timestamp
        >> this->message
        >> this->pending_count;
}

ServerProfileTable::Profile::Profile() : notif_counter(0)
{
}

void ServerProfileTable::Profile::serialize(Serializer& stream) const
{
    stream
        << this->notif_counter
        << this->username
        << this->followers
        << this->received_notifs
        << this->pending_notifs;
}

void ServerProfileTable::Profile::deserialize(Deserializer& stream)
{
    stream
        >> this->notif_counter
        >> this->username
        >> this->followers
        >> this->received_notifs
        >> this->pending_notifs;
}

ServerProfileTable::ServerProfileTable()
{
}

void ServerProfileTable::serialize(Serializer& stream) const
{
    stream << this->profiles;
}

void ServerProfileTable::deserialize(Deserializer& stream)
{
    std::unique_lock lock(this->control_mutex);
    stream >> this->profiles;
}
