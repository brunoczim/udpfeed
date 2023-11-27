#ifndef SHARED_MESSAGE_H_
#define SHARED_MESSAGE_H_ 1

#include <cstdint>
#include <string>
#include <memory>

class InvalidMessagePayload: public std::exception {
    private:
        std::string error_message;
    public:
        InvalidMessagePayload(std::string const& error_message);

        virtual const char *what() const noexcept;
};

class MessageSerializer {
    private:
        std::string bytes;
    public:
        std::string const& finish() const;

        MessageSerializer& operator<<(char const *data);

        MessageSerializer& operator<<(std::string const& data);

        MessageSerializer& operator<<(uint64_t data);

        MessageSerializer& operator<<(int64_t data);
};

class MessageDeserializer {
    private:
        std::string const& bytes;
        size_t i;
    public:
        MessageDeserializer(std::string const& bytes);

        MessageDeserializer& operator>>(std::string &data);

        MessageDeserializer& operator>>(uint64_t &data);

        MessageDeserializer& operator>>(int64_t &data);

    private:
        uint8_t next();
};

class Serializable {
    public:
        virtual void serialize(MessageSerializer& serializer) const = 0;

        virtual void deserialize(MessageDeserializer& deserializer) = 0;

        virtual ~Serializable();
};

enum MessageType {
    MSG_CONNECT_REQ,
    MSG_CONNECT_RESP
};

struct MessageHeader : public Serializable {
    uint64_t seqn;
    uint64_t timestamp;

    static MessageHeader create();

    virtual void serialize(MessageSerializer& serializer) const;

    virtual void deserialize(MessageDeserializer& deserializer);
};

class MessageBody : public Serializable {
    public:
        virtual MessageType type() = 0;
        virtual ~MessageBody();
};

struct Message : public Serializable {
    MessageHeader header;
    std::shared_ptr<MessageBody> body;

    virtual void serialize(MessageSerializer& serializer) const;

    virtual void deserialize(MessageDeserializer& deserializer);
};

MessageSerializer& operator<<(
    MessageSerializer& serializer,
    Serializable const& serializable
);

MessageDeserializer& operator>>(
    MessageDeserializer& deserializer,
    Serializable& serializable
);

#endif
