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

std::vector<Address> ServerGroup::load_server_addr_list(char const *path)
{
    std::ifstream file;
    file.open(path);
    if (file.fail()) {
        return std::vector<Address>();
    }

    std::string buf;
    std::vector<Address> addresses;

    while (std::getline(file, buf)) {
        buf = trim_spaces(buf);
        if (!buf.empty()) {
            size_t pos = buf.find(':');
            if (pos == std::string::npos) {
                pos = buf.find(' ');
            }
            if (pos == std::string::npos) {
                throw ServerListFailure(path);
            }
            std::string ipv4_str = buf.substr(0, pos);
            std::string port_str = buf.substr(pos + 1, buf.size());
            uint32_t ipv4 = parse_ipv4(ipv4_str);
            uint16_t port = parse_udp_port(port_str);
            addresses.push_back(Address(ipv4, port));
        }
    }

    return addresses;
}

std::vector<Address> ServerGroup::load_server_addr_list()
{
    char const *var = getenv(ServerGroup::path_env_var);
    if (var == NULL) {
        return ServerGroup::load_server_addr_list(ServerGroup::default_path);
    } 
    return ServerGroup::load_server_addr_list(var);
}
