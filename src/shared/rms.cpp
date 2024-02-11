#include "rms.h"

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

RmGroup::RmGroup() : RmGroup(std::set<Address>(), std::nullopt)
{
}

RmGroup::RmGroup(
    std::set<Address> const& servers,
    std::optional<Address> primary
) :
    servers(servers),
    primary(primary)
{
    if (
        this->primary.has_value()
        && this->servers.find(*this->primary) == this->servers.cend()
    ) {
        throw UnknownServerAddr(*primary);
    }
}

std::set<Address> const& RmGroup::server_addrs() const
{
    return this->servers;
}

std::optional<Address> RmGroup::primary_addr() const
{
    return this->primary;
}

bool RmGroup::contains_server(Address server_addr) const
{
    return
        this->server_addrs().find(server_addr) != this->server_addrs().cend();
}

bool RmGroup::add_server(Address server_addr)
{
    return std::get<1>(this->servers.insert(server_addr));
}

bool RmGroup::remove_server(Address server_addr)
{
    return !this->servers.extract(server_addr).empty();
}

void RmGroup::primary_elected(Address primary_addr)
{
    this->primary = primary_addr;
}

void RmGroup::primary_failed()
{
    this->primary = std::nullopt;
}

void RmGroup::serialize(Serializer& stream) const
{
    stream << this->servers << this->primary;
}

void RmGroup::deserialize(Deserializer& stream)
{
    stream >> this->servers >> this->primary;
}
