#ifndef SERVER_GROUP_H_
#define SERVER_GROUP_H_ 1

#include <set>
#include <optional>
#include <exception>
#include <string>
#include "../shared/address.h"
#include "../shared/serialization.h"
#include "../shared/socket.h"

class UnknownServerAddr {
    private:
        Address server;
        std::string message;
    public:
        UnknownServerAddr(Address server_addr);
        Address server_addr() const;
        virtual char const *what() const noexcept;
};

class ServerListFailure {
    private:
        std::string path_;
        std::string message;
    public:
        ServerListFailure(std::string const& path);
        std::string const& path() const;
        virtual char const *what() const noexcept;
};

class ServerGroup : public Serializable, public Deserializable {
    private:
        std::set<Address> servers;
        Address self;
        std::optional<Address> coordinator;

    public:
        static constexpr char const *path_env_var = "SISOP2_SERVER_GROUP_FILE";

        static constexpr char const *direct_env_var = "SISOP2_SERVER_GROUP";

        static constexpr char const *default_path = ".sisop2_server_addrs";

        ServerGroup(Address self);
        bool add_server(Address server_addr);
        bool remove_server(Address server_addr);
        void elected(Address coordinator_addr);
        std::optional<Address> coordinator_addr() const;
        std::set<Address> const &server_addrs() const;

        virtual void serialize(Serializer& stream) const;
        virtual void deserialize(Deserializer& stream);

        void connect(ReliableSocket& socket, char const *path = NULL);

    private:
        bool try_connect(ReliableSocket& socket, Address server_addr);

    public:

        static char const *server_addr_list_path(
            char const *default_path = NULL
        );

        static std::set<Address> load_server_addr_list(
            char const *default_path = NULL
        );
};

#endif
