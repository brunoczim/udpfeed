#include <fstream>
#include <sstream>
#include "data.h"
#include "../shared/log.h"

ServerProfileTable::Notification::Notification() :
    Notification(0, NotifMessage(), 0, 0)
{
}

ServerProfileTable::Notification::Notification(
    uint64_t id,
    NotifMessage message,
    int64_t timestamp,
    uint64_t pending_count
) :
    id(id),
    message(message),
    sent_at(timestamp),
    pending_count(pending_count)
{
}

ServerProfileTable::Profile::Profile(Username username, int64_t timestamp) :
    username(username),
    created_at(timestamp),
    notif_counter(0)
{
    this->username = username;
}

ServerProfileTable::Profile::Profile() : Profile(Username(), 0)
{
}

void ServerProfileTable::Profile::serialize(Serializer& stream) const
{
    stream
        << this->username
        << this->created_at
        << this->followers
    ;
}

void ServerProfileTable::Profile::deserialize(Deserializer& stream)
{
    stream
        >> this->username
        >> this->created_at
        >> this->followers
    ;
}

ServerProfileTable::ServerProfileTable() : active(true), dirty(false)
{
    char const *var = getenv(ServerProfileTable::path_env_var);
    if (var == NULL) {
        this->unsafe_set_path(ServerProfileTable::default_path);
    } else {
        this->unsafe_set_path(var);
    }
}

ServerProfileTable::ServerProfileTable(std::string const& path) :
    active(true),
    dirty(false)
{
    this->unsafe_set_path(path);
}

void ServerProfileTable::unsafe_set_path(std::string const& path)
{
    this->path = path;
    Logger::with([&path] (auto& output) {
        output
            << "Using file with path "
            << path
            << " to persist server data"
            << std::endl;
    });
}

void ServerProfileTable::unsafe_set_dirty()
{
    this->dirty = true;
    this->persistence_cond_var.notify_one();
}

void ServerProfileTable::connect(
    Address client,
    Username const& profile_username,
    int64_t timestamp
)
{
    std::unique_lock lock(this->data_control_mutex);
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

    this->unsafe_set_dirty();
}

bool ServerProfileTable::disconnect(Address client, int64_t timestamp)
{
    bool disconnected = false;
    std::unique_lock lock(this->data_control_mutex);

    if (auto node = this->sessions.extract(client)) {
        this->profiles[node.mapped()].sessions.erase(client);
        disconnected = true;
    }

    this->unsafe_set_dirty();
    return disconnected;
}

void ServerProfileTable::follow(
    Address client,
    Username const& followed_username,
    int64_t timestamp
)
{
    std::unique_lock lock(this->data_control_mutex);

    auto follower_node = this->sessions.find(client);
    if (follower_node == this->sessions.end()) {
        throw ThrowableMessageError(MSG_NO_CONNECTION);
    }
    auto followed_node = this->profiles.find(followed_username);
    if (followed_node == this->profiles.end()) {
        throw ThrowableMessageError(MSG_UNKNOWN_USERNAME);
    }
    Username follower_username = std::get<1>(*follower_node);
    if (follower_username == followed_username) {
        throw ThrowableMessageError(MSG_CANNOT_FOLLOW_SELF);
    }
    Profile& followed = std::get<1>(*followed_node);
    followed.followers.insert(follower_username);

    this->unsafe_set_dirty();
}

void ServerProfileTable::notify(
    Address client,
    NotifMessage message,
    Channel<Username>::Sender& followers_sender,
    int64_t timestamp
)
{
    std::unique_lock lock(this->data_control_mutex);

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
    uint64_t notif_id = sender.notif_counter;
    uint64_t pending_count = sender.followers.size();

    sender.received_notifs.insert(std::make_pair(
        notif_id,
        Notification(notif_id, message, timestamp, pending_count)
    ));

    for (auto const& follower_username : sender.followers) {
        auto followed_node = this->profiles.find(follower_username);
        if (followed_node == this->profiles.end()) {
            throw ThrowableMessageError(MSG_UNKNOWN_USERNAME);
        }
        followers_sender.send(follower_username);
        Profile& follower = std::get<1>(*followed_node);
        follower.pending_notifs.push_back(std::make_pair(
            sender_username,
            notif_id
        ));
    }
}

std::optional<PendingNotif> ServerProfileTable::consume_one_notif(
    Username username
)
{
    std::unique_lock lock(this->data_control_mutex);
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
        pending_notif.sent_at = notif.sent_at;
        notif.pending_count--;
        delivered_to_all = notif.pending_count == 0;
    }
    if (delivered_to_all) {
        followed.received_notifs.erase(notif_id);
    }

    return std::make_optional(pending_notif);
}

bool ServerProfileTable::persist_on_dirty()
{
    bool dirty = false;
    bool active = true;
    std::ostringstream sstream;
    {
        PlaintextSerializer serializer_impl(sstream);
        Serializer& serializer = serializer_impl;

        std::unique_lock lock(this->data_control_mutex);

        while (this->active && !this->dirty) {
            this->persistence_cond_var.wait(lock);
        }

        dirty = this->dirty;
        this->dirty = false;
        active = this->active;

        serializer << *this;
    }

    if (dirty) {
        std::unique_lock lock(this->file_control_mutex);
        std::ofstream file;

        file.open(path, std::ios::out | std::ios::trunc | std::ios::binary);

        file << sstream.str() << std::endl << std::flush;

        if (!file.good() && !file.eof()) {
            Logger::with([] (auto& output) {
                output
                    << "Failed to persist server data"
                    << std::endl;
            });
        }

        file.close();
    }

    return active;
}

bool ServerProfileTable::load()
{
    bool success = false;

    Logger::with([&path = this->path] (auto& output) {
        output
            << "Will attempt to load server data from "
            << path
            << std::endl;
    });

    std::unique_lock lock(this->file_control_mutex);
    std::ifstream file;

    file.open(path, std::ios::in | std::ios::binary);

    if (file.is_open()) {
        PlaintextDeserializer deserializer_impl(file);
        Deserializer& deserializer = deserializer_impl;

        try {
            deserializer >> *this;
            deserializer.ensure_eof();
            success = file.good() || file.eof();
        } catch (DeserializationError const& exc) {
            Logger::with([&exc] (auto& output) {
                output
                    << "Failed to deserialize server data: "
                    << exc.what()
                    << std::endl;
            });
        }

        file.close();
    }

    if (success) {
        Logger::with([] (auto& output) {
            output << "Loaded server data from file" << std::endl;
        });
    } else {
        Logger::with([] (auto& output) {
            output << "Did not load server data file" << std::endl;
        });
        this->profiles.clear();
    }

    return success;
}

void ServerProfileTable::shutdown()
{
    std::unique_lock lock(this->data_control_mutex);
    this->active = false;
    this->persistence_cond_var.notify_all();
}

void ServerProfileTable::serialize(Serializer& stream) const
{
    stream << this->profiles;
}

void ServerProfileTable::deserialize(Deserializer& stream)
{
    std::unique_lock lock(this->data_control_mutex);
    stream >> this->profiles;
}
