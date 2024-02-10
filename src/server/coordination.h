#ifndef SERVER_GROUP_H_
#define SERVER_GROUP_H_ 1

#include <set>
#include <optional>
#include <exception>
#include <string>
#include "../shared/address.h"
#include "../shared/serialization.h"
#include "../shared/socket.h"
#include "../shared/rms.h"

class ServerListFailure : public std::exception {
    private:
        std::string path_;
        std::string message;
    public:
        ServerListFailure(std::string const& path);
        std::string const& path() const;
        virtual char const *what() const noexcept;
};

class SelfServerRemoved : public std::exception {
    public:
        virtual char const *what() const noexcept;
};

class ServerCoordination {
    public:
        enum ElectionState {
            ELECTED,
            ELECTION_REQUIRED,
            WAITING_RESULTS,
            CANDIDATE
        };

    private:
        RmGroup group_;
        Address self;
        ElectionState election_state;

    public:
        static constexpr char const *path_env_var = "SISOP2_SERVER_GROUP_FILE";

        static constexpr char const *direct_env_var = "SISOP2_SERVER_GROUP";

        static constexpr char const *default_path = ".sisop2_server_addrs";

        ServerCoordination(Address self);
        ServerCoordination(Address self, RmGroup const& group);

        RmGroup const& group() const;
        Address self_addr() const;
        ServerCoordination::ElectionState cur_election_state() const;
        bool add_server(Address address);
        bool remove_server(Address address);

        std::set<Address> bullies() const;

        void connect(ReliableSocket& socket, char const *path = NULL);

    private:
        bool try_connect(ReliableSocket& socket, Address server_addr);

        void load_data(
            std::set<Address> const& servers,
            std::optional<Address> coordinator
        );

    public:

        static char const *server_addr_list_path(
            char const *default_path = NULL
        );

        static std::set<Address> load_server_addr_list(
            char const *default_path = NULL
        );
};

#endif
