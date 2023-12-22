#ifndef SHARED_SERIALIZATION_H_
#define SHARED_SERIALIZATION_H_ 1

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#include <deque>
#include <set>
#include <map>

class Serializable;

class Deserializable;

class SerializationError : public std::exception {
    protected:
        std::string message;

    public:
        SerializationError(std::string const& message);

        virtual const char *what() const noexcept;

        virtual ~SerializationError();
};

class DeserializationError : public std::exception {
    protected:
        std::string message;

    public:
        DeserializationError(std::string const& message);

        virtual const char *what() const noexcept;

        virtual ~DeserializationError();
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

        template <typename T>
        Serializer& operator<<(std::deque<T> const& data);

        template <typename T>
        Serializer& operator<<(std::set<T> const& data);

        template <typename K, typename V>
        Serializer& operator<<(std::map<K, V> const& data);

        template <typename A, typename B>
        Serializer& operator<<(std::pair<A, B> const& data);

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
        
        template <typename T>
        Deserializer& operator>>(std::deque<T>& data);

        template <typename T>
        Deserializer& operator>>(std::set<T>& data);

        template <typename K, typename V>
        Deserializer& operator>>(std::map<K, V>& data);

        template <typename A, typename B>
        Deserializer& operator>>(std::pair<A, B>& data);

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
    *this << (uint32_t) data.size();
    for (T const& element : data) {
        *this << element;
    }
    return *this;
}

template <typename T>
Serializer& Serializer::operator<<(std::deque<T> const& data)
{
    *this << (uint32_t) data.size();
    for (T const& element : data) {
        *this << element;
    }
    return *this;
}

template <typename T>
Serializer& Serializer::operator<<(std::set<T> const& data)
{
    *this << (uint32_t) data.size();
    for (T const& element : data) {
        *this << element;
    }
    return *this;
}

template <typename K, typename V>
Serializer& Serializer::operator<<(std::map<K, V> const& data)
{
    *this << (uint32_t) data.size();
    for (auto const& entry : data) {
        K const& key = std::get<0>(entry);
        V const& value = std::get<1>(entry);
        *this << key << value;
    }
    return *this;
}

template <typename A, typename B>
Serializer& Serializer::operator<<(std::pair<A, B> const& data)
{
    *this << std::get<0>(data) << std::get<1>(data);
    return *this;
}

template <typename T>
Deserializer& Deserializer::operator>>(std::vector<T>& data)
{
    uint32_t size;
    *this >> size;
    data.resize(size);
    for (T& element : data) {
        *this >> element;
    }
    return *this;
}

template <typename T>
Deserializer& Deserializer::operator>>(std::deque<T>& data)
{
    uint32_t size;
    *this >> size;
    data.clear();
    for (uint32_t i = 0; i < size; i++) {
        T element;
        *this >> element;
        data.push_back(std::move(element));
    }
    return *this;
}

template <typename T>
Deserializer& Deserializer::operator>>(std::set<T>& data)
{
    uint32_t size;
    *this >> size;
    data.clear();
    for (uint32_t i = 0; i < size; i++) {
        T element;
        *this >> element;
        data.insert(std::move(element));
    }
    return *this;
}

template <typename K, typename V>
Deserializer& Deserializer::operator>>(std::map<K, V>& data)
{
    uint32_t size;
    *this >> size;
    data.clear();
    for (uint32_t i = 0; i < size; i++) {
        K key;
        V value;
        *this >> key >> value;
        data.insert(std::make_pair(std::move(key), std::move(value)));
    }
    return *this;
}

template <typename A, typename B>
Deserializer& Deserializer::operator>>(std::pair<A, B>& data)
{
    *this >> std::get<0>(data) >> std::get<1>(data);
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
