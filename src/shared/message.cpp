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

InvalidMessageType::InvalidMessageType(uint16_t code) :
    message("invalid message type code: "),
    code_(code)
{
    this->message += std::to_string(code);
}

uint16_t InvalidMessageType::code() const
{
    return this->code_;
}

const char *InvalidMessageType::what() const noexcept
{
    return this->message.c_str();
}

MessageType msg_type_from_code(uint16_t code)
{
    switch (code) {
        case MSG_ACK: return MSG_ACK;
        case MSG_CONNECT_REQ: return MSG_CONNECT_REQ;
        case MSG_CONNECT_RESP: return MSG_CONNECT_RESP;
        case MSG_DISCONNECT: return MSG_DISCONNECT;
        case MSG_PING: return MSG_PING;
        default: throw InvalidMessageType(code);
    }
}

Serializer& operator<<(Serializer& serializer, MessageType type)
{
    uint16_t code = type;
    serializer << code;
    return serializer;
}

Deserializer& operator>>(Deserializer& deserializer, MessageType &type)
{
    uint16_t code;
    deserializer >> code;
    type = msg_type_from_code(code);
    return deserializer;
}

InvalidMessageStatus::InvalidMessageStatus(uint16_t code) :
    message("invalid message status code: "),
    code_(code)
{
    this->message += std::to_string(code);
}

uint16_t InvalidMessageStatus::code() const
{
    return this->code_;
}

const char *InvalidMessageStatus::what() const noexcept
{
    return this->message.c_str();
}

MessageStatus msg_status_from_code(uint16_t code)
{
    switch (code) {
        case MSG_OK: return MSG_OK;
        case MSG_BAD_USERNAME: return MSG_BAD_USERNAME;
        case MSG_TOO_MANY_SESSIONS: return MSG_TOO_MANY_SESSIONS;
        default: throw InvalidMessageStatus(code);
    }
}

Serializer& operator<<(Serializer& serializer, MessageStatus status)
{
    uint16_t code = status;
    serializer << code;
    return serializer;
}

Deserializer& operator>>(Deserializer& deserializer, MessageStatus& status)
{
    uint16_t code;
    deserializer >> code;
    status = msg_status_from_code(code);
    return deserializer;
}

MessageHeader MessageHeader::create()
{
    static std::atomic<uint64_t> current_seqn = 0;
    MessageHeader header;
    header.seqn = current_seqn.fetch_add(1);
    header.timestamp = time(NULL);
    return header;
}

void MessageHeader::serialize(Serializer& serializer) const
{
    serializer << this->seqn << this->timestamp;
}

void MessageHeader::deserialize(Deserializer& deserializer)
{
    deserializer >> this->seqn >> this->timestamp;
}

MessageBody::~MessageBody()
{
}

MessageType MessageAck::type()
{
    return MSG_ACK;
}

void MessageAck::serialize(Serializer& serializer) const
{
}

void MessageAck::deserialize(Deserializer& deserializer)
{
}

MessageConnectReq::MessageConnectReq() : MessageConnectReq(std::string())
{
}

MessageConnectReq::MessageConnectReq(std::string username) : username(username)
{
}

MessageType MessageConnectReq::type()
{
    return MSG_CONNECT_REQ;
}

void MessageConnectReq::serialize(Serializer& serializer) const
{
    serializer << this->username;
}

void MessageConnectReq::deserialize(Deserializer& deserializer)
{
    deserializer >> this->username;
}

MessageConnectResp::MessageConnectResp() : MessageConnectResp(MSG_OK)
{
}

MessageConnectResp::MessageConnectResp(MessageStatus status) : status(status)
{
}

MessageType MessageConnectResp::type()
{
    return MSG_CONNECT_RESP;
}

void MessageConnectResp::serialize(Serializer& serializer) const
{
    serializer << (uint64_t) this->status;
}

void MessageConnectResp::deserialize(Deserializer& deserializer)
{
    uint64_t code;
    deserializer >> code;
    this->status = msg_status_from_code(code);
}

MessageType MessageDisconnect::type()
{
    return MSG_DISCONNECT;
}

void MessageDisconnect::serialize(Serializer& serializer) const
{
}

void MessageDisconnect::deserialize(Deserializer& deserializer)
{
}

MessageType MessagePing::type()
{
    return MSG_PING;
}

void MessagePing::serialize(Serializer& serializer) const
{
}

void MessagePing::deserialize(Deserializer& deserializer)
{
}

void Message::serialize(Serializer& serializer) const
{
    uint64_t code = this->body->type();
    serializer << this->header << code << *this->body;
}

void Message::deserialize(Deserializer& deserializer)
{
    deserializer >> this->header;
    uint16_t code;
    deserializer >> code;
    MessageType type = msg_type_from_code(code);
    switch (type) {
        case MSG_ACK:
            this->body =
                std::shared_ptr<MessageBody>(new MessageAck);
            break;
        case MSG_CONNECT_REQ:
            this->body =
                std::shared_ptr<MessageBody>(new MessageConnectReq);
            break;
        case MSG_CONNECT_RESP:
            this->body =
                std::shared_ptr<MessageBody>(new MessageConnectResp);
            break;
            break;
    }
    deserializer >> *this->body;
}
