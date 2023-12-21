#ifndef SHARED_MESSAGE_H_
#define SHARED_MESSAGE_H_ 1

#include "serialization.h"
#include "address.h"

#include <cstdint>
#include <string>
#include <memory>

constexpr uint64_t MSG_MAGIC_NUMBER = 8969265839344830156;

class MessageOutOfProtocol : public std::exception {
    public:
        virtual const char *what() const noexcept;
};

class InvalidMessagePayload: public std::exception {
    private:
        std::string error_message;
    public:
        InvalidMessagePayload(std::string const& error_message);

        virtual const char *what() const noexcept;
};

enum MessageError {
    MSG_OK,
    MSG_INTERNAL_ERR,
    MSG_NO_CONNECTION,
    MSG_OUTDATED_SEQN,
    MSG_BAD_USERNAME,
    MSG_TOO_MANY_SESSIONS
};

class InvalidMessageError : public std::exception {
    private:
        std::string message;
        uint16_t code_;
    public:
        InvalidMessageError(uint16_t code);

        uint16_t code() const;

        virtual const char *what() const noexcept;
};

MessageError msg_status_from_code(uint16_t code);

Serializer& operator<<(Serializer& serializer, MessageError status);
Deserializer& operator>>(Deserializer& deserializer, MessageError& status);

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
    MSG_ERROR,
    MSG_CONNECT,
    MSG_DISCONNECT,
    MSG_FOLLOW,
    MSG_NOTIFY
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
        int64_t timestamp;

        MessageHeader();

        void fill_req();
        void fill_resp(uint64_t seqn);

        virtual void serialize(Serializer& serializer) const;
        virtual void deserialize(Deserializer& deserializer);
};

class CastOnMessageError : public std::exception {
    private:
        MessageError error_;
        std::string message;
    public:
        CastOnMessageError(MessageError error);
        MessageError const& error() const;
        virtual const char *what() const noexcept;
};

class MessageBody : public Serializable, public Deserializable {
    public:
        virtual MessageTag tag() const = 0;

        template <typename T>
        T& cast();

        template <typename T>
        T const& cast() const;

        virtual ~MessageBody();
};

class MessageErrorResp : public MessageBody {
    public:
        MessageError error;

        MessageErrorResp();
        MessageErrorResp(MessageError error);

        virtual MessageTag tag() const;

        virtual void serialize(Serializer& serializer) const;
        virtual void deserialize(Deserializer& deserializer);
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

class MessageFollowReq : public MessageBody {
    public:
        std::string username;

        MessageFollowReq();
        MessageFollowReq(std::string username);

        virtual MessageTag tag() const;

        virtual void serialize(Serializer& serializer) const;
        virtual void deserialize(Deserializer& deserializer);
};

class MessageFollowResp : public MessageBody {
    public:
        virtual MessageTag tag() const;

        virtual void serialize(Serializer& serializer) const;
        virtual void deserialize(Deserializer& deserializer);
};

class MessageNotifyReq : public MessageBody {
    public:
        std::string notification;

        MessageNotifyReq();
        MessageNotifyReq(std::string notification);

        virtual MessageTag tag() const;

        virtual void serialize(Serializer& serializer) const;
        virtual void deserialize(Deserializer& deserializer);
};

class MessageNotifyResp : public MessageBody {
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

class Enveloped {
    public:
        Enveloped();
        Enveloped(Address remote, Message message);

        Address remote;
        Message message;
};

template <typename T>
T& MessageBody::cast()
{
    try {
        return dynamic_cast<T&>(*this);
    } catch (std::bad_cast const& exception) {
        throw CastOnMessageError(
            dynamic_cast<MessageErrorResp const&>(*this).error
        );
    }
}

template <typename T>
T const& MessageBody::cast() const
{
    try {
        return dynamic_cast<T const&>(*this);
    } catch (std::bad_cast const& exception) {
        throw CastOnMessageError(
            dynamic_cast<MessageErrorResp const&>(*this).error
        );
    }
}

#endif
