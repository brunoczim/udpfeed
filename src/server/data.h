#ifndef SERVER_PROFILE_H_
#define SERVER_PROFILE_H_ 1

#include <string>
#include <set>
#include <map>
#include <deque>
#include <mutex>
#include "../shared/address.h"
#include "../shared/username.h"
#include "../shared/notif_message.h"
#include "../shared/serialization.h"
#include "../shared/channel.h"

class ServerProfileTable : public Serializable, public Deserializable {
    private:
        class Notification : public Serializable, public Deserializable {
            public:
                uint64_t id;
                int64_t timestamp;
                NotifMessage message;
                uint64_t pending_count;

                Notification();

                virtual void serialize(Serializer& stream) const;
                virtual void deserialize(Deserializer& stream);
        };

        class Profile : public Serializable, public Deserializable {
            private:
                uint64_t notif_counter;
                Username username;
                std::map<Username, int64_t> followers;
                std::map<uint64_t, Notification> received_notifs;
                std::set<Address> sessions;
                std::deque<std::pair<Username, uint64_t>> pending_notifs;

            public:
                Profile();

                virtual void serialize(Serializer& stream) const;
                virtual void deserialize(Deserializer& stream);
        };

        std::mutex control_mutex;
        std::map<Username, Profile> profiles;
        std::map<Address, Username> sessions;
    public:
        ServerProfileTable();

        virtual void serialize(Serializer& stream) const;
        virtual void deserialize(Deserializer& stream);
};



#endif
