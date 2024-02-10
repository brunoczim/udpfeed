#include "group.h"
#include "../shared/log.h"
#include "../shared/string_ext.h"
#include <fstream>

UnknownServerAddr::UnknownServerAddr(Address server_addr) :
    server(server_addr),
    message("Server address " + server_addr.to_string() + " is unknown")
{
}

Address UnknownServerAddr::server_addr() const
{
    return this->server;
}

char const *UnknownServerAddr::what() const noexcept
{
    return this->message.c_str();
}

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

ServerGroup::ServerGroup(Address self) : self(self), coordinator(std::nullopt)
{
}

bool ServerGroup::add_server(Address server_addr)
{
    return std::get<1>(this->servers.insert(server_addr));
}

bool ServerGroup::remove_server(Address server_addr)
{
    if (this->servers.extract(server_addr)) {
        if (this->coordinator && server_addr == *this->coordinator) {
            this->coordinator = std::nullopt;
        }
        return true;
    }
    return false;
}

void ServerGroup::elected(Address coordinator_addr)
{
    if (this->servers.find(coordinator_addr) == this->servers.end()) {
        throw UnknownServerAddr(coordinator_addr);
    }
    this->coordinator = coordinator_addr;
}

Address ServerGroup::self_addr() const
{
    return this->self;
}

std::optional<Address> ServerGroup::coordinator_addr() const
{
    return this->coordinator;
}

std::set<Address> const &ServerGroup::server_addrs() const
{
    return this->servers;
}

ServerGroup::ElectionState ServerGroup::cur_election_state() const
{
    return this->election_state;
}

std::set<Address> ServerGroup::bullies() const
{
    auto iterator = this->servers.upper_bound(this->self);
    return std::set<Address>(iterator, this->servers.cend());
}

void ServerGroup::serialize(Serializer& stream) const
{
    stream << this->servers << this->coordinator;
}

void ServerGroup::deserialize(Deserializer& stream)
{
    stream >> this->servers >> this->coordinator;
}

void ServerGroup::connect(ReliableSocket& socket, char const *path)
{
    this->servers.clear();
    this->coordinator = std::nullopt;

    std::set<Address> list = ServerGroup::load_server_addr_list(path);

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
        this->elected(this->self);
    }
}

bool ServerGroup::try_connect(ReliableSocket& socket, Address server_addr)
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

void ServerGroup::load_data(
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

    this->servers = servers;
    this->coordinator = coordinator;
}

char const *ServerGroup::server_addr_list_path(char const *default_path)
{
    char const *path = default_path;
    if (path == NULL) {
        path = getenv(ServerGroup::path_env_var);
        if (path[0] == 0) {
            path = NULL;
        }
    }
    if (path == NULL) {
        path = ServerGroup::default_path;
    }
    return path;
}

std::set<Address> ServerGroup::load_server_addr_list(char const *default_path)
{
    char const *path = ServerGroup::server_addr_list_path(default_path);

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


    char const *direct_addr = getenv(ServerGroup::direct_env_var);
    if (direct_addr != NULL && direct_addr[0] != 0) {
        Logger::with([] (auto& output) {
            output
                << "Getting a server group alternative from environemnt "
                << "variable "
                << ServerGroup::direct_env_var
                << std::endl;
        });
        addresses.insert(Address::parse(direct_addr));
    }

    return addresses;
}
