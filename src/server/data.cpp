#include "data.h"

ServerProfileTable::Notification::Notification() :
    id(0),
    pending_count(0)
{
}

void ServerProfileTable::Notification::serialize(Serializer& stream) const
{
    stream
        << this->id
        << this->message
        << this->pending_count;
}

void ServerProfileTable::Notification::deserialize(Deserializer& stream)
{
    stream
        >> this->id
        >> this->message
        >> this->pending_count;
}

ServerProfileTable::Profile::Profile(Username username, int64_t timestamp) :
    Profile()
{
    this->username = username;
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

void ServerProfileTable::connect(
    Address client,
    Username const& profile_username,
    int64_t timestamp
)
{
    std::unique_lock lock(this->control_mutex);
    if (this->profiles.find(profile_username) == this->profiles.end()) {
        this->profiles.insert(std::make_pair(
            profile_username,
            Profile(profile_username, timestamp)
        ));
    }

    Profile& profile = this->profiles[profile_username];
    if (profile.sessions.size() >= ServerProfileTable::MAX_SESSIONS_PER_PROF) {
        throw ThrowableMessageError(MSG_TOO_MANY_SESSIONS);
    }

    profile.sessions.insert(client);
    this->sessions.insert(std::make_pair(client, profile_username));
}

void ServerProfileTable::disconnect(Address client, int64_t timestamp)
{
    std::unique_lock lock(this->control_mutex);

    if (auto node = this->sessions.extract(client)) {
        this->profiles[node.mapped()].sessions.erase(client);
    }
}

void ServerProfileTable::follow(
    Address client,
    Username const& followed_username,
    int64_t timestamp
)
{
    std::unique_lock lock(this->control_mutex);

    auto follower_node = this->sessions.find(client);
    if (follower_node == this->sessions.end()) {
        throw ThrowableMessageError(MSG_NO_CONNECTION);
    }
    auto followed_node = this->profiles.find(followed_username);
    if (followed_node == this->profiles.end()) {
        throw ThrowableMessageError(MSG_UNKNOWN_USERNAME);
    }
    Username follower_username = std::get<1>(*follower_node);
    Profile& followed = std::get<1>(*followed_node);
    followed.followers.insert(follower_username);
}

void ServerProfileTable::notify(
    Address client,
    NotifMessage message,
    Channel<Username>::Sender& followers_sender,
    int64_t timestamp
)
{
    std::unique_lock lock(this->control_mutex);

    auto sender_sesssion_node = this->sessions.find(client);
    if (sender_sesssion_node == this->sessions.end()) {
        throw ThrowableMessageError(MSG_NO_CONNECTION);
    }

    Username const& sender_username = std::get<1>(*sender_sesssion_node);

    auto sender_node = this->profiles.find(sender_username);
    if (sender_node == this->profiles.end()) {
        throw ThrowableMessageError(MSG_UNKNOWN_USERNAME);
    }

    Profile& sender = std::get<1>(*sender_node);
    sender.notif_counter++;
    Notification notif;
    notif.id = sender.notif_counter;
    notif.message = message;
    notif.pending_count = sender.followers.size();

    sender.received_notifs.insert(std::make_pair(notif.id, notif));

    for (auto const& follower_username : sender.followers) {
        auto followed_node = this->profiles.find(follower_username);
        if (followed_node == this->profiles.end()) {
            throw ThrowableMessageError(MSG_UNKNOWN_USERNAME);
        }
        followers_sender.send(follower_username);
        Profile& follower = std::get<1>(*followed_node);
        follower.pending_notifs.push_back(std::make_pair(
            sender_username,
            notif.id
        ));
    }

}

std::optional<PendingNotif> ServerProfileTable::consume_one_notif(
    Username username
)
{
    std::unique_lock lock(this->control_mutex);
    Profile& follower = this->profiles[username];
    if (follower.pending_notifs.empty()) {
        return std::optional<PendingNotif>();
    }

    std::pair<Username, uint64_t> notif_pair =
        std::move(follower.pending_notifs.front());
    follower.pending_notifs.pop_front();

    Username followed_username = std::move(std::get<0>(notif_pair));
    uint64_t notif_id = std::get<1>(notif_pair);
    Profile& followed = this->profiles[followed_username];

    PendingNotif pending_notif;
    pending_notif.sender = followed_username;
    pending_notif.receivers = follower.sessions;
    bool delivered_to_all = false;
    {
        ServerProfileTable::Notification& notif =
            followed.received_notifs[notif_id];
        pending_notif.message = notif.message;
        notif.pending_count--;
        delivered_to_all = notif.pending_count == 0;
    }
    if (delivered_to_all) {
        followed.received_notifs.erase(notif_id);
    }

    return std::make_optional(pending_notif);
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
