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
        class Notification {
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

        std::mutex data_control_mutex;
        std::condition_variable persistence_cond_var;
        std::mutex file_control_mutex;
        bool active;
        bool dirty;
        std::map<Username, Profile> profiles;
        std::map<Address, Username> sessions;
        std::string path;

        void unsafe_set_path(std::string const& path);

        void unsafe_set_dirty();

    public:
        static constexpr size_t MAX_SESSIONS_PER_PROF = 2;

        static constexpr char const *path_env_var = "SISOP2_PERSIST_FILE";

        static constexpr char const *default_path = ".sisop2_udpfeed_data";

        ServerProfileTable();
        ServerProfileTable(std::string const& path);

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

        void notify(
            Address client,
            NotifMessage message,
            Channel<Username>::Sender& followers_sender,
            int64_t timestamp
        );

        std::optional<PendingNotif> consume_one_notif(Username username);

        bool persist_on_dirty();

        bool load();

        void shutdown();

        virtual void serialize(Serializer& stream) const;
        virtual void deserialize(Deserializer& stream);
};

#endif
