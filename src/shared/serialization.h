#ifndef SHARED_SERIALIZATION_H_
#define SHARED_SERIALIZATION_H_ 1

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

class Serializable;

class Deserializable;

class SerializationError : public std::exception {
    private:
        std::string message;

    public:
        SerializationError(std::string const& message);

        virtual const char *what() const noexcept;
};

class DeserializationError : public std::exception {
    private:
        std::string message;

    public:
        DeserializationError(std::string const& message);

        virtual const char *what() const noexcept;
};

class DeserializationUnexpectedEof : public DeserializationError {
    public:
        DeserializationUnexpectedEof();
};

class DeserializationExpectedEof : public DeserializationError {
    public:
        DeserializationExpectedEof();
};

class Serializer {
    protected:
        std::ostream &stream;
    public:
        Serializer(std::ostream& stream);

        virtual Serializer& operator<<(bool data) = 0;
        virtual Serializer& operator<<(uint8_t data) = 0;
        virtual Serializer& operator<<(uint16_t data) = 0;
        virtual Serializer& operator<<(uint32_t data) = 0;
        virtual Serializer& operator<<(uint64_t data) = 0;

        virtual Serializer& operator<<(int8_t data) = 0;
        virtual Serializer& operator<<(int16_t data) = 0;
        virtual Serializer& operator<<(int32_t data) = 0;
        virtual Serializer& operator<<(int64_t data) = 0;

        virtual Serializer& operator<<(char const *data);
        virtual Serializer& operator<<(std::string const& data) = 0;

        template <typename T>
        Serializer& operator<<(std::vector<T> const& data);

        virtual Serializer& operator<<(Serializable const& data);

        virtual ~Serializer();
};

class Deserializer {
    protected:
        std::istream &stream;
    public:
        Deserializer(std::istream& stream);

        virtual Deserializer& operator>>(bool& data) = 0;
        virtual Deserializer& operator>>(uint8_t& data) = 0;
        virtual Deserializer& operator>>(uint16_t& data) = 0;
        virtual Deserializer& operator>>(uint32_t& data) = 0;
        virtual Deserializer& operator>>(uint64_t& data) = 0;

        virtual Deserializer& operator>>(int8_t& data) = 0;
        virtual Deserializer& operator>>(int16_t& data) = 0;
        virtual Deserializer& operator>>(int32_t& data) = 0;
        virtual Deserializer& operator>>(int64_t& data) = 0;

        virtual Deserializer& operator>>(std::string& data) = 0;

        template <typename T>
        Deserializer& operator>>(std::vector<T>& data);

        virtual Deserializer& operator>>(Deserializable& data);

        virtual void ensure_eof();

        virtual ~Deserializer();
};

class Serializable {
    public:
        virtual void serialize(Serializer& stream) const = 0;
        virtual ~Serializable();
};

class Deserializable {
    public:
        virtual void deserialize(Deserializer& stream) = 0;
        virtual ~Deserializable();
};

template <typename T>
Serializer& Serializer::operator<<(std::vector<T> const& data)
{
    *this << (uint16_t) data.size();
    for (T const& element : data) {
        *this << element;
    }
    return *this;
}

template <typename T>
Deserializer& Deserializer::operator>>(std::vector<T>& data)
{
    uint16_t size;
    *this >> size;
    data.resize(size);
    for (T& element : data) {
        *this >> element;
    }
    return *this;
}

class PlaintextSerializer : public Serializer {
    public:
        PlaintextSerializer(std::ostream& stream);

        virtual Serializer& operator<<(bool data);
        virtual Serializer& operator<<(uint8_t data);
        virtual Serializer& operator<<(uint16_t data);
        virtual Serializer& operator<<(uint32_t data);
        virtual Serializer& operator<<(uint64_t data);

        virtual Serializer& operator<<(int8_t data);
        virtual Serializer& operator<<(int16_t data);
        virtual Serializer& operator<<(int32_t data);
        virtual Serializer& operator<<(int64_t data);

        virtual Serializer& operator<<(std::string const& data);
};

class PlaintextInvalidInt : public DeserializationError {
    private:
        std::string type_;
        std::string content_;

    public:
        PlaintextInvalidInt(
            std::string const& type,
            std::string const& content
        );
        char const *type() const;
        char const *content() const;
};

class PlaintextDeserializer : public Deserializer {
    public:
        PlaintextDeserializer(std::istream& stream);

        virtual Deserializer& operator>>(bool& data);
        virtual Deserializer& operator>>(uint8_t& data);
        virtual Deserializer& operator>>(uint16_t& data);
        virtual Deserializer& operator>>(uint32_t& data);
        virtual Deserializer& operator>>(uint64_t& data);

        virtual Deserializer& operator>>(int8_t& data);
        virtual Deserializer& operator>>(int16_t& data);
        virtual Deserializer& operator>>(int32_t& data);
        virtual Deserializer& operator>>(int64_t& data);

        virtual Deserializer& operator>>(std::string& data);
};

#endif
