#ifndef SHARED_MESSAGE_H_
#define SHARED_MESSAGE_H_ 1

#include "serialization.h"
#include "username.h"
#include "notif_message.h"
#include "address.h"

#include <cstdint>
#include <string>
#include <memory>

constexpr uint64_t MSG_MAGIC_NUMBER = 8969265839344830156;

class MessageOutOfProtocol : public std::exception {
    public:
        virtual const char *what() const noexcept;
};

class InvalidMessagePayload : public DeserializationError {
    public:
        InvalidMessagePayload(std::string const& error_message);
};

enum MessageError {
    MSG_INTERNAL_ERR,
    MSG_BAD,
    MSG_MISSED_RESP,
    MSG_NO_CONNECTION,
    MSG_OUTDATED_SEQN,
    MSG_UNKNOWN_USERNAME,
    MSG_TOO_MANY_SESSIONS,
    MSG_CANNOT_FOLLOW_SELF
};

class ThrowableMessageError : public std::exception {
    private:
        MessageError error_;

    public:
        ThrowableMessageError(MessageError error);

        MessageError error() const;

        virtual char const *what() const noexcept;
};


class InvalidMessageError : public DeserializationError {
    private:
        uint16_t code_;
    public:
        InvalidMessageError(uint16_t code);

        uint16_t code() const;
};

MessageError msg_error_from_code(uint16_t code);

char const *msg_error_render(MessageError error);

Serializer& operator<<(Serializer& serializer, MessageError status);
Deserializer& operator>>(Deserializer& deserializer, MessageError& status);

enum MessageStep {
    MSG_REQ,
    MSG_RESP
};

class InvalidMessageStep : public DeserializationError {
    private:
        uint16_t code_;
    public:
        InvalidMessageStep(uint16_t code);

        uint16_t code() const;
};

MessageStep msg_step_from_code(uint16_t code);

Serializer& operator<<(Serializer& serializer, MessageStep step);
Deserializer& operator>>(Deserializer& deserializer, MessageStep &step);

enum MessageType {
    MSG_ERROR,
    MSG_CLIENT_CONN,
    MSG_DISCONNECT,
    MSG_PING,
    MSG_FOLLOW,
    MSG_NOTIFY,
    MSG_DELIVER
};

class InvalidMessageType : public DeserializationError {
    private:
        uint16_t code_;
    public:
        InvalidMessageType(uint16_t code);

        uint16_t code() const;
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

class MessageClientConnReq : public MessageBody {
    public:
        Username username;

        MessageClientConnReq();
        MessageClientConnReq(Username const& username);

        virtual MessageTag tag() const;

        virtual void serialize(Serializer& serializer) const;
        virtual void deserialize(Deserializer& deserializer);
};

class MessageClientConnResp : public MessageBody {
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

class MessagePingReq : public MessageBody {
    public:
        virtual MessageTag tag() const;

        virtual void serialize(Serializer& serializer) const;
        virtual void deserialize(Deserializer& deserializer);
};

class MessagePingResp : public MessageBody {
    public:
        virtual MessageTag tag() const;

        virtual void serialize(Serializer& serializer) const;
        virtual void deserialize(Deserializer& deserializer);
};

class MessageFollowReq : public MessageBody {
    public:
        Username username;

        MessageFollowReq();
        MessageFollowReq(Username const& username);

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
        NotifMessage notif_message;

        MessageNotifyReq();
        MessageNotifyReq(NotifMessage const& notif_msg);

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

class MessageDeliverReq : public MessageBody {
    public:
        Username sender;
        NotifMessage notif_message;
        int64_t sent_at;

        MessageDeliverReq();
        MessageDeliverReq(
            Username const& sender,
            NotifMessage const& notif_msg,
            int64_t sent_at
        );

        virtual MessageTag tag() const;

        virtual void serialize(Serializer& serializer) const;
        virtual void deserialize(Deserializer& deserializer);
};

class MessageDeliverResp : public MessageBody {
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
