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

enum MessageType {
    MSG_REQ_ACK,
    MSG_RESP_ACK,
    MSG_CONNECT_REQ,
    MSG_CONNECT_RESP
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

MessageType msg_type_from_code(uint16_t code);

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

class MessageHeader : public Serializable, public Deserializable {
    public:
        uint64_t seqn;
        uint64_t timestamp;

        static MessageHeader create();

        virtual void serialize(Serializer& serializer) const;

        virtual void deserialize(Deserializer& deserializer);
};

class MessageBody : public Serializable, public Deserializable {
    public:
        virtual MessageType type() = 0;
        virtual ~MessageBody();
};

class MessageConnectReq : public MessageBody {
    public:
        std::string username;

        MessageConnectReq();
        MessageConnectReq(std::string username);

        virtual MessageType type();

        virtual void serialize(Serializer& serializer) const;

        virtual void deserialize(Deserializer& deserializer);
};

class MessageConnectResp : public MessageBody {
    public:
        MessageStatus status;

        MessageConnectResp();
        MessageConnectResp(MessageStatus status);

        virtual MessageType type();

        virtual void serialize(Serializer& serializer) const;

        virtual void deserialize(Deserializer& deserializer);
};

class MessageReqAck : public MessageBody {
    public:
        virtual MessageType type();

        virtual void serialize(Serializer& serializer) const;

        virtual void deserialize(Deserializer& deserializer);
};

class MessageRespAck : public MessageBody {
    public:
        virtual MessageType type();

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
