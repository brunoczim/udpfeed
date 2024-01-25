#include "group.h"
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

std::optional<Address> ServerGroup::coordinator_addr() const
{
    return this->coordinator;
}

std::set<Address> const &ServerGroup::server_addrs() const
{
    return this->servers;
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
    Enveloped resp_enveloped = request.receive_resp();
    try {
        resp_enveloped.body->cast<MessageServerConnResp>();
        return true;
    } catch (MissedResponse const& exc) {
        return false;
    }
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

    std::ifstream file;
    file.open(path);
    if (file.fail()) {
        return std::vector<Address>();
    }

    std::string buf;
    std::set<Address> addresses;

    while (std::getline(file, buf)) {
        buf = trim_spaces(buf);
        if (!buf.empty()) {
            addresses.insert(Address::parse(buf));
        }
    }

    char const *direct_addr = getenv(ServerGroup::direct_env_var);
    if (direct_addr != NULL && direct_addr[0] == 0) {
        addresses.insert(Address::parse(direct_addr));
    }

    return addresses;
}
