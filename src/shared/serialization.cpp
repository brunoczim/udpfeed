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

Serializable::~Serializable()
{
}

Deserializable::~Deserializable()
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

PlaintextInvalidInt::PlaintextInvalidInt(
    std::string const& type,
    std::string const& content
) :
    DeserializationError(
        "found invalid integer " + content + " for type " + type
    ),
    type_(type),
    content_(content)
{
}

char const *PlaintextInvalidInt::type() const
{
    return this->type_.c_str();
}

char const *PlaintextInvalidInt::content() const
{
    return this->content_.c_str();
}

PlaintextDeserializer::PlaintextDeserializer(std::istream& stream) :
    Deserializer(stream)
{
}

Deserializer& PlaintextDeserializer::operator>>(uint8_t& data)
{
    uint64_t bigger;
    *this >> bigger;
    if (bigger > UINT8_MAX) {
        throw PlaintextInvalidInt("uint8", std::to_string(bigger));
    }
    data = bigger;
    return *this;
}

Deserializer& PlaintextDeserializer::operator>>(uint16_t& data)
{
    uint64_t bigger;
    *this >> bigger;
    if (bigger > UINT16_MAX) {
        throw PlaintextInvalidInt("uint16", std::to_string(bigger));
    }
    data = bigger;
    return *this;
}

Deserializer& PlaintextDeserializer::operator>>(uint32_t& data)
{
    uint64_t bigger;
    *this >> bigger;
    if (bigger > UINT32_MAX) {
        throw PlaintextInvalidInt("uint32", std::to_string(bigger));
    }
    data = bigger;
    return *this;
}

Deserializer& PlaintextDeserializer::operator>>(uint64_t& data)
{
    std::string buf;
    *this >> buf;
    if (buf.empty()) {
        throw PlaintextInvalidInt("uint64", buf);
    }
    char *end;
    long long unsigned integer = strtoull(buf.c_str(), &end, 10);
    if (*end != '\0') {
        throw PlaintextInvalidInt("uint64", buf);
    }
    data = integer;
    return *this;
}

Deserializer& PlaintextDeserializer::operator>>(int8_t& data)
{
    int64_t bigger;
    *this >> bigger;
    if (bigger < INT8_MIN || bigger > INT8_MAX) {
        throw PlaintextInvalidInt("int8", std::to_string(bigger));
    }
    data = bigger;
    return *this;
}

Deserializer& PlaintextDeserializer::operator>>(int16_t& data)
{
    int64_t bigger;
    *this >> bigger;
    if (bigger < INT16_MIN || bigger > INT16_MAX) {
        throw PlaintextInvalidInt("int16", std::to_string(bigger));
    }
    data = bigger;
    return *this;
}

Deserializer& PlaintextDeserializer::operator>>(int32_t& data)
{
    int64_t bigger;
    *this >> bigger;
    if (bigger < INT32_MIN || bigger > INT32_MAX) {
        throw PlaintextInvalidInt("int32", std::to_string(bigger));
    }
    data = bigger;
    return *this;
}

Deserializer& PlaintextDeserializer::operator>>(int64_t& data)
{
    std::string buf;
    *this >> buf;
    if (buf.empty()) {
        throw PlaintextInvalidInt("int64", buf);
    }
    char *end;
    long long signed integer = strtoll(buf.c_str(), &end, 10);
    if (*end != '\0') {
        throw PlaintextInvalidInt("int64", buf);
    }
    data = integer;
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
