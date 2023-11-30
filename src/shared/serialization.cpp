#include <sstream>
#include "serialization.h"

SerializationError::SerializationError(std::string const& message) :
    message(message)
{
}

const char *SerializationError::what() const noexcept
{
    return message.c_str();
}

DeserializationError::DeserializationError(std::string const& message) :
    message(message)
{
}

const char *DeserializationError::what() const noexcept
{
    return message.c_str();
}

DeserializationExpectedEof::DeserializationExpectedEof() : DeserializationError(
    "expected end of input during deserialization but got more data"
)
{
}

Serializer::Serializer(std::ostream& stream) : stream(stream)
{
}

Serializer& Serializer::operator<<(Serializable const& data)
{
    data.serialize(*this);
    return *this;
}

Serializer::~Serializer()
{
}

Deserializer::Deserializer(std::istream& stream) : stream(stream)
{
}

Deserializer& Deserializer::operator>>(Deserializable& data)
{
    data.deserialize(*this);
    return *this;
}

void Deserializer::ensure_eof()
{
    if (!this->stream.eof()) {
        throw DeserializationExpectedEof();
    }
}

Deserializer::~Deserializer()
{
}

PlaintextSerializer::PlaintextSerializer(std::ostream& stream) :
    Serializer(stream)
{
}

Serializer& PlaintextSerializer::operator<<(uint8_t data)
{
    *this << std::to_string(data);
    return *this;
}

Serializer& PlaintextSerializer::operator<<(uint16_t data)
{
    *this << std::to_string(data);
    return *this;
}

Serializer& PlaintextSerializer::operator<<(uint32_t data)
{
    *this << std::to_string(data);
    return *this;
}

Serializer& PlaintextSerializer::operator<<(uint64_t data)
{
    *this << std::to_string(data);
    return *this;
}

Serializer& PlaintextSerializer::operator<<(int8_t data)
{
    *this << std::to_string(data);
    return *this;
}

Serializer& PlaintextSerializer::operator<<(int16_t data)
{
    *this << std::to_string(data);
    return *this;
}

Serializer& PlaintextSerializer::operator<<(int32_t data)
{
    *this << std::to_string(data);
    return *this;
}

Serializer& PlaintextSerializer::operator<<(int64_t data)
{
    *this << std::to_string(data);
    return *this;
}

Serializer& PlaintextSerializer::operator<<(std::string const& data)
{
    for (char ch : data) {
        switch (ch) {
            case ';':
            case '\\':
                this->stream << '\\';
            default:
                this->stream << ch;
                break;
        }
    }
    this->stream << ';';
    return *this;
}

Deserializer& PlaintextDeserializer::operator>>(uint8_t& data)
{
    std::string buf;
    *this >> buf;
    std::istringstream istream(buf);
    istream >> data;
    return *this;
}

Deserializer& PlaintextDeserializer::operator>>(uint16_t& data)
{
    std::string buf;
    *this >> buf;
    std::istringstream istream(buf);
    istream >> data;
    return *this;
}

Deserializer& PlaintextDeserializer::operator>>(uint32_t& data)
{
    std::string buf;
    *this >> buf;
    std::istringstream istream(buf);
    istream >> data;
    return *this;
}

Deserializer& PlaintextDeserializer::operator>>(uint64_t& data)
{
    std::string buf;
    *this >> buf;
    std::istringstream istream(buf);
    istream >> data;
    return *this;
}

Deserializer& PlaintextDeserializer::operator>>(int8_t& data)
{
    std::string buf;
    *this >> buf;
    std::istringstream istream(buf);
    istream >> data;
    return *this;
}

Deserializer& PlaintextDeserializer::operator>>(int16_t& data)
{
    std::string buf;
    *this >> buf;
    std::istringstream istream(buf);
    istream >> data;
    return *this;
}

Deserializer& PlaintextDeserializer::operator>>(int32_t& data)
{
    std::string buf;
    *this >> buf;
    std::istringstream istream(buf);
    istream >> data;
    return *this;
}

Deserializer& PlaintextDeserializer::operator>>(int64_t& data)
{
    std::string buf;
    *this >> buf;
    std::istringstream istream(buf);
    istream >> data;
    return *this;
}

Deserializer& PlaintextDeserializer::operator>>(std::string& data)
{
    data.erase();
    int byte;
    while ((byte = this->stream.get()) != ';' && byte != EOF) {
        if (byte == '\\') {
            byte = this->stream.get();
            if (byte == EOF) {
                break;
            }
        }
        data.push_back(byte);
    }
    if (byte == EOF) {
        throw DeserializationError("unexpected end of input");
    }
    return *this;
}
