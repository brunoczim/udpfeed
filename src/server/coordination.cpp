#include "coordination.h"
#include "../shared/log.h"
#include "../shared/string_ext.h"
#include <fstream>

ServerListFailure::ServerListFailure(std::string const& path) :
    path_(path),
    message("Couldn't read server list from " + path)
{
}

std::string const& ServerListFailure::path() const
{
    return this->path_;
}

char const *ServerListFailure::what() const noexcept
{
    return this->message.c_str();
}

char const *SelfServerRemoved::what() const noexcept
{
    return "bad server coordination state, self is not in group";
}

ServerCoordination::ServerCoordination(Address self) :
    ServerCoordination(self, RmGroup(std::set<Address> { self }, std::nullopt))
{
}

ServerCoordination::ServerCoordination(Address self, RmGroup const& group) :
    self(self),
    group_(group)
{
    if (!this->group_.contains_server(self)) {
        throw SelfServerRemoved();
    }
}

Address ServerCoordination::self_addr() const
{
    return this->self;
}

RmGroup const& ServerCoordination::group() const
{
    return this->group_;
}

ServerCoordination::ElectionState ServerCoordination::cur_election_state() const
{
    return this->election_state;
}

bool ServerCoordination::add_server(Address address)
{
    return this->group_.add_server(address);
}

bool ServerCoordination::remove_server(Address address)
{
    if (address == this->self) {
        throw SelfServerRemoved();
    }
    return this->group_.remove_server(address);
}

std::set<Address> ServerCoordination::bullies() const
{
    auto iterator = this->group().server_addrs().upper_bound(this->self);
    return std::set<Address>(iterator, this->group().server_addrs().cend());
}

void ServerCoordination::connect(ReliableSocket& socket, char const *path)
{
    std::set<Address> list = ServerCoordination::load_server_addr_list(path);

    bool connected = false;
    bool attempted = false;

    for (auto server_addr : list) {
        attempted = true;
        connected = this->try_connect(socket, server_addr);
        if (connected) {
            break;
        }
    }

    if (!connected) {
        if (attempted) {
            throw ServerListFailure("could not connect to any given server");
        }
        this->group_.primary_elected(this->self);
    }
}

bool ServerCoordination::try_connect(ReliableSocket& socket, Address server_addr)
{
    Enveloped req_enveloped;
    req_enveloped.remote = server_addr;
    req_enveloped.message.body =
        std::shared_ptr<MessageBody>(new MessageServerConnReq);
    ReliableSocket::SentReq request = socket.send_req(req_enveloped);
    Enveloped resp_enveloped = std::move(request).receive_resp();
    try {
        MessageServerConnResp resp_data =
            resp_enveloped.message.body->cast<MessageServerConnResp>();
        this->load_data(resp_data.members, resp_data.coordinator);
        return true;
    } catch (MissedResponse const& exc) {
        return false;
    }
}

void ServerCoordination::load_data(
    std::set<Address> const& servers,
    std::optional<Address> coordinator
)
{
    if (coordinator && servers.find(*coordinator) == servers.end()) {
        throw ServerListFailure(
            "coordinator " + coordinator->to_string() + " is not in server list"
        );
    }

    if (servers.find(this->self) == servers.end()) {
        throw ServerListFailure(
            "this server's address "
                + this->self.to_string()
                + " is not in server list"
        );
    }

    this->group_ = RmGroup(servers, coordinator);
}

char const *ServerCoordination::server_addr_list_path(char const *default_path)
{
    char const *path = default_path;
    if (path == NULL) {
        path = getenv(ServerCoordination::path_env_var);
        if (path[0] == 0) {
            path = NULL;
        }
    }
    if (path == NULL) {
        path = ServerCoordination::default_path;
    }
    return path;
}

std::set<Address> ServerCoordination::load_server_addr_list(char const *default_path)
{
    char const *path = ServerCoordination::server_addr_list_path(default_path);

    std::set<Address> addresses;

    Logger::with([path] (auto& output) {
        output
            << "Looking for server group addresses in file "
            << path
            << std::endl;
    });

    std::ifstream file;
    file.open(path);
    if (file.good()) {
        std::string buf;
        while (std::getline(file, buf)) {
            buf = trim_spaces(buf);
            if (!buf.empty()) {
                addresses.insert(Address::parse(buf));
            }
        }
    } else {
        Logger::with([path] (auto& output) {
            output
                << "Failed to open file "
                << path
                << "... Skipping it"
                << std::endl;
        });
    }


    char const *direct_addr = getenv(ServerCoordination::direct_env_var);
    if (direct_addr != NULL && direct_addr[0] != 0) {
        Logger::with([] (auto& output) {
            output
                << "Getting a server group alternative from environemnt "
                << "variable "
                << ServerCoordination::direct_env_var
                << std::endl;
        });
        addresses.insert(Address::parse(direct_addr));
    }

    return addresses;
}
