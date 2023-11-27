#include "message.h"
#include <sstream>
#include <ctime>
#include <atomic>

InvalidMessagePayload::InvalidMessagePayload(std::string const& error_message) :
    error_message(error_message)
{
}

const char *InvalidMessagePayload::what() const noexcept
{
    return this->error_message.c_str();
}

std::string const& MessageSerializer::finish() const
{
    return this->bytes;
}

MessageSerializer& MessageSerializer::operator<<(char const *data)
{
    for (size_t i = 0; data[i] != 0; i++) {
        switch (data[i]) {
            case ';':
            case '\\':
                this->bytes.push_back('\\');
            default:
                this->bytes.push_back(data[i]);
                break;
        }
    }
    this->bytes.push_back(';');
    return *this;
}

MessageSerializer& MessageSerializer::operator<<(std::string const& data)
{
    *this << data.c_str();
    return *this;
}

MessageSerializer& MessageSerializer::operator<<(uint64_t data)
{
    std::stringstream sstream;
    sstream << data;
    *this << sstream.str();
    return *this;
}

MessageSerializer& MessageSerializer::operator<<(int64_t data)
{
    std::stringstream sstream;
    sstream << data;
    *this << sstream.str();
    return *this;
}

MessageDeserializer::MessageDeserializer(std::string const& bytes) :
    bytes(bytes),
    i(0)
{
}

MessageDeserializer& MessageDeserializer::operator>>(std::string &data)
{
    data.erase();
    uint8_t byte;
    while ((byte = this->next()) != ';') {
        if (byte == '\\') {
            byte = this->next();
        }
        data.push_back(byte);
    }
    return *this;
}

MessageDeserializer& MessageDeserializer::operator>>(uint64_t& data)
{
    std::string buf;
    *this >> buf;
    std::istringstream istream(buf);
    istream >> data;
    return *this;
}

MessageDeserializer& MessageDeserializer::operator>>(int64_t& data)
{
    std::string buf;
    *this >> buf;
    std::istringstream istream(buf);
    istream >> data;
    return *this;
}

uint8_t MessageDeserializer::next()
{
    if (this->i >= this->bytes.size()) {
        throw InvalidMessagePayload("requested field that does not exist");
    }
    uint8_t byte = this->bytes[this->i];
    this->i++;
    return byte;
}

Serializable::~Serializable()
{
}

MessageHeader MessageHeader::create()
{
    static std::atomic<uint64_t> current_seqn = 0;
    MessageHeader header;
    header.seqn = current_seqn.fetch_add(1);
    header.timestamp = time(NULL);
    return header;
}

void MessageHeader::serialize(MessageSerializer& serializer) const
{
    serializer << this->seqn << this->timestamp;
}

void MessageHeader::deserialize(MessageDeserializer& deserializer)
{
    deserializer >> this->seqn >> this->timestamp;
}

MessageBody::~MessageBody()
{
}

void Message::serialize(MessageSerializer& serializer) const
{
    serializer << this->header << *this->body;
}

void Message::deserialize(MessageDeserializer& deserializer)
{
    deserializer >> this->header >> *this->body;
}

MessageSerializer& operator<<(
    MessageSerializer& serializer,
    Serializable const& serializable
)
{
    serializable.serialize(serializer);
    return serializer;
}

MessageDeserializer& operator>>(
    MessageDeserializer& deserializer,
    Serializable& serializable
)
{
    serializable.deserialize(deserializer);
    return deserializer;
}
