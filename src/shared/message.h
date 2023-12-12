#ifndef SHARED_MESSAGE_H_
#define SHARED_MESSAGE_H_ 1

#include "serialization.h"

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

enum MessageStatus {
    MSG_OK,
    MSG_BAD_USERNAME,
    MSG_TOO_MANY_SESSIONS
};

class InvalidMessageStatus : public std::exception {
    private:
        std::string message;
        uint16_t code_;
    public:
        InvalidMessageStatus(uint16_t code);

        uint16_t code() const;

        virtual const char *what() const noexcept;
};

MessageStatus msg_status_from_code(uint16_t code);

Serializer& operator<<(Serializer& serializer, MessageStatus status);
Deserializer& operator>>(Deserializer& deserializer, MessageStatus& status);

enum MessageStep {
    MSG_REQ,
    MSG_RESP
};

class InvalidMessageStep : public std::exception {
    private:
        std::string message;
        uint16_t code_;
    public:
        InvalidMessageStep(uint16_t code);

        uint16_t code() const;

        virtual const char *what() const noexcept;
};

MessageStep msg_step_from_code(uint16_t code);

Serializer& operator<<(Serializer& serializer, MessageStep step);
Deserializer& operator>>(Deserializer& deserializer, MessageStep &step);

enum MessageType {
    MSG_CONNECT,
    MSG_DISCONNECT
};

class InvalidMessageType : public std::exception {
    private:
        std::string message;
        uint16_t code_;
    public:
        InvalidMessageType(uint16_t code);

        uint16_t code() const;

        virtual const char *what() const noexcept;
};

MessageType msg_tag_from_code(uint16_t code);

Serializer& operator<<(Serializer& serializer, MessageType tag);
Deserializer& operator>>(Deserializer& deserializer, MessageType &tag);

class MessageTag : public Serializable, public Deserializable {
    public:
        MessageStep step;
        MessageType type;

        MessageTag();
        MessageTag(MessageStep step, MessageType type);

        bool operator==(MessageTag const& other) const;
        bool operator!=(MessageTag const& other) const;

        bool operator<(MessageTag const& other) const;
        bool operator<=(MessageTag const& other) const;
        bool operator>(MessageTag const& other) const;
        bool operator>=(MessageTag const& other) const;

        std::string to_string() const;

        virtual void serialize(Serializer& serializer) const;
        virtual void deserialize(Deserializer& deserializer);
};

class MessageHeader : public Serializable, public Deserializable {
    public:
        uint64_t seqn;
        uint64_t timestamp;

        static MessageHeader gen_request();
        static MessageHeader gen_response(uint64_t seqn);

        virtual void serialize(Serializer& serializer) const;
        virtual void deserialize(Deserializer& deserializer);
};

class MessageBody : public Serializable, public Deserializable {
    public:
        virtual MessageTag tag() const = 0;
        virtual ~MessageBody();
};

class MessageConnectReq : public MessageBody {
    public:
        std::string username;

        MessageConnectReq();
        MessageConnectReq(std::string username);

        virtual MessageTag tag() const;

        virtual void serialize(Serializer& serializer) const;
        virtual void deserialize(Deserializer& deserializer);
};

class MessageConnectResp : public MessageBody {
    public:
        MessageStatus status;

        MessageConnectResp();
        MessageConnectResp(MessageStatus status);

        virtual MessageTag tag() const;

        virtual void serialize(Serializer& serializer) const;
        virtual void deserialize(Deserializer& deserializer);
};

class MessageDisconnectReq : public MessageBody {
    public:
        virtual MessageTag tag() const;

        virtual void serialize(Serializer& serializer) const;
        virtual void deserialize(Deserializer& deserializer);
};

class MessageDisconnectResp : public MessageBody {
    public:
        virtual MessageTag tag() const;

        virtual void serialize(Serializer& serializer) const;
        virtual void deserialize(Deserializer& deserializer);
};

class Message : public Serializable, public Deserializable {
    public:
        MessageHeader header;
        std::shared_ptr<MessageBody> body;

        virtual void serialize(Serializer& serializer) const;
        virtual void deserialize(Deserializer& deserializer);
};

#endif
