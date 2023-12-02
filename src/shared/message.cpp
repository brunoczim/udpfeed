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
        case MSG_CONNECT_REQ: return MSG_CONNECT_REQ;
        case MSG_CONNECT_RESP: return MSG_CONNECT_RESP;
        case MSG_REQ_ACK: return MSG_REQ_ACK;
        case MSG_RESP_ACK: return MSG_RESP_ACK;
        default: throw InvalidMessageType(code);
    }
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

MessageType MessageReqAck::type()
{
    return MSG_REQ_ACK;
}

void MessageReqAck::serialize(Serializer& serializer) const
{
}

void MessageReqAck::deserialize(Deserializer& deserializer)
{
}

void MessageRespAck::serialize(Serializer& serializer) const
{
}

void MessageRespAck::deserialize(Deserializer& deserializer)
{
}

MessageType MessageRespAck::type()
{
    return MSG_RESP_ACK;
}

void Message::serialize(Serializer& serializer) const
{
    uint64_t code = this->body->type();
    serializer << this->header << code << *this->body;
}

void Message::deserialize(Deserializer& deserializer)
{
    uint16_t code;
    deserializer >> this->header >> code;
    MessageType type = msg_type_from_code(code);
    switch (type) {
        case MSG_CONNECT_REQ:
            this->body = std::shared_ptr<MessageBody>(new MessageConnectReq);
            break;
        case MSG_CONNECT_RESP:
            this->body = std::shared_ptr<MessageBody>(new MessageConnectResp);
            break;
        case MSG_REQ_ACK:
            this->body = std::shared_ptr<MessageBody>(new MessageReqAck);
            break;
        case MSG_RESP_ACK:
            this->body = std::shared_ptr<MessageBody>(new MessageRespAck);
            break;
    }
    deserializer >> *this->body;
}
