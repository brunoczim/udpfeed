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
        int64_t sent_at;
        std::set<Address> receivers;
};

class ServerProfileTable : public Serializable, public Deserializable {
    private:
        class Notification : public Serializable, public Deserializable {
            public:
                uint64_t id;
                NotifMessage message;
                int64_t sent_at;
                uint64_t pending_count;

                Notification();
                Notification(
                    uint64_t id,
                    NotifMessage message,
                    int64_t timestamp,
                    uint64_t pending_count
                );

                virtual void serialize(Serializer& stream) const;
                virtual void deserialize(Deserializer& stream);
        };

        class Profile : public Serializable, public Deserializable {
            public:
                uint64_t notif_counter;
                Username username;
                int64_t created_at;
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

        static constexpr char const *file_env_var = "SISOP2_PERSIST_FILE";

        static constexpr char const *default_file = ".sisop2_udpfeed_data";

        ServerProfileTable();

        void connect(
            Address client,
            Username const& profile,
            Channel<Username>::Sender& followers_sender,
            int64_t timestamp
        );

        void disconnect(Address client, int64_t timestamp);

        void follow(
            Address client,
            Username const& followed,
            int64_t timestamp
        );

        void notify(
            Address client,
            NotifMessage message,
            Channel<Username>::Sender& followers_sender,
            int64_t timestamp
        );

        std::optional<PendingNotif> consume_one_notif(Username username);

        void persist(std::string const& path) const;
        void persist() const;

        bool load(std::string const& path);
        bool load();

        virtual void serialize(Serializer& stream) const;
        virtual void deserialize(Deserializer& stream);
};

#endif
