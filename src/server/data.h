#ifndef SERVER_PROFILE_H_
#define SERVER_PROFILE_H_ 1

#include <string>
#include <set>
#include <map>
#include <deque>
#include <mutex>
#include "../shared/message.h"
#include "../shared/address.h"
#include "../shared/username.h"
#include "../shared/notif_message.h"
#include "../shared/serialization.h"
#include "../shared/channel.h"

class PendingNotif {
    public:
        Username sender;
        NotifMessage message;
        std::set<Address> receivers;
};

class ServerProfileTable : public Serializable, public Deserializable {
    private:
        class Notification : public Serializable, public Deserializable {
            public:
                uint64_t id;
                NotifMessage message;
                uint64_t pending_count;

                Notification();

                virtual void serialize(Serializer& stream) const;
                virtual void deserialize(Deserializer& stream);
        };

        class Profile : public Serializable, public Deserializable {
            public:
                uint64_t notif_counter;
                Username username;
                std::set<Username> followers;
                std::map<uint64_t, Notification> received_notifs;
                std::set<Address> sessions;
                std::deque<std::pair<Username, uint64_t>> pending_notifs;

                Profile();
                Profile(Username username, int64_t timestamp);

                virtual void serialize(Serializer& stream) const;
                virtual void deserialize(Deserializer& stream);
        };

        std::mutex control_mutex;
        std::map<Username, Profile> profiles;
        std::map<Address, Username> sessions;
    public:
        static constexpr size_t MAX_SESSIONS_PER_PROF = 2;

        ServerProfileTable();

        void connect(
            Address client,
            Username const& profile,
            int64_t timestamp
        );

        void disconnect(Address client, int64_t timestamp);

        void follow(
            Address client,
            Username const& followed,
            int64_t timestamp
        );

        std::optional<PendingNotif> read_pending_notif(Username username);

        virtual void serialize(Serializer& stream) const;
        virtual void deserialize(Deserializer& stream);
};



#endif
