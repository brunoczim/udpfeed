#include "message.h"
#include <sstream>
#include <ctime>
#include <atomic>

const char *MessageOutOfProtocol::what() const noexcept
{
    return "message does not have the magic number thus not recognized";
}

InvalidMessagePayload::InvalidMessagePayload(std::string const& error_message) :
    DeserializationError(error_message)
{
}

InvalidMessageStep::InvalidMessageStep(uint16_t code) :
    DeserializationError("invalid message step code: "),
    code_(code)
{
    this->message += std::to_string(code);
}

uint16_t InvalidMessageStep::code() const
{
    return this->code_;
}

MessageStep msg_step_from_code(uint16_t code)
{
    switch (code) {
        case MSG_REQ: return MSG_REQ;
        case MSG_RESP: return MSG_RESP;
        default: throw InvalidMessageStep(code);
    }
}

Serializer& operator<<(Serializer& serializer, MessageStep step)
{
    uint16_t code = step;
    serializer << code;
    return serializer;
}

Deserializer& operator>>(Deserializer& deserializer, MessageStep &step)
{
    uint16_t code;
    deserializer >> code;
    step = msg_step_from_code(code);
    return deserializer;
}

InvalidMessageType::InvalidMessageType(uint16_t code) :
    DeserializationError("invalid message type code: "),
    code_(code)
{
    this->message += std::to_string(code);
}

uint16_t InvalidMessageType::code() const
{
    return this->code_;
}

MessageType msg_type_from_code(uint16_t code)
{
    switch (code) {
        case MSG_ERROR: return MSG_ERROR;
        case MSG_CLIENT_CONN: return MSG_CLIENT_CONN;
        case MSG_DISCONNECT: return MSG_DISCONNECT;
        case MSG_PING: return MSG_PING;
        case MSG_FOLLOW: return MSG_FOLLOW;
        case MSG_NOTIFY: return MSG_NOTIFY;
        case MSG_DELIVER: return MSG_DELIVER;
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

ThrowableMessageError::ThrowableMessageError(MessageError error) : error_(error)
{
}

MessageError ThrowableMessageError::error() const
{
    return this->error_;
}

char const *ThrowableMessageError::what() const noexcept
{
    return msg_error_render(this->error());
}

InvalidMessageError::InvalidMessageError(uint16_t code) :
    DeserializationError("invalid message error code: "),
    code_(code)
{
    this->message += std::to_string(code);
}

uint16_t InvalidMessageError::code() const
{
    return this->code_;
}

MessageError msg_error_from_code(uint16_t code)
{
    switch (code) {
        case MSG_INTERNAL_ERR: return MSG_INTERNAL_ERR;
        case MSG_MISSED_RESP: return MSG_MISSED_RESP;
        case MSG_NO_CONNECTION: return MSG_NO_CONNECTION;
        case MSG_OUTDATED_SEQN: return MSG_OUTDATED_SEQN;
        case MSG_UNKNOWN_USERNAME: return MSG_UNKNOWN_USERNAME;
        case MSG_TOO_MANY_SESSIONS: return MSG_TOO_MANY_SESSIONS;
        case MSG_CANNOT_FOLLOW_SELF: return MSG_CANNOT_FOLLOW_SELF;
        default: throw InvalidMessageError(code);
    }
}

char const *msg_error_render(MessageError error)
{
    switch (error) {
        case MSG_INTERNAL_ERR: return "internal error";
        case MSG_MISSED_RESP: return "missed message response";
        case MSG_NO_CONNECTION: return "no connection estabilished";
        case MSG_OUTDATED_SEQN: return "message sequence number is outdated";
        case MSG_UNKNOWN_USERNAME: return "given username is unknown";
        case MSG_TOO_MANY_SESSIONS: return "profile has too many sessions";
        case MSG_CANNOT_FOLLOW_SELF: return "profile cannot follow itself";
        default: return "unknown error";
    }
}

Serializer& operator<<(Serializer& serializer, MessageError error)
{
    uint16_t code = error;
    serializer << code;
    return serializer;
}

Deserializer& operator>>(Deserializer& deserializer, MessageError& error)
{
    uint16_t code;
    deserializer >> code;
    error = msg_error_from_code(code);
    return deserializer;
}

MessageTag::MessageTag() : MessageTag(MSG_REQ, MSG_CLIENT_CONN)
{
}

MessageTag::MessageTag(MessageStep step, MessageType type) :
    step(step),
    type(type)
{
}

bool MessageTag::operator==(MessageTag const& other) const
{
    return this->step == other.step && this->type == other.type;
}

bool MessageTag::operator!=(MessageTag const& other) const
{
    return this->step != other.step || this->type != other.type;
}

bool MessageTag::operator<(MessageTag const& other) const
{
    if (this->step < other.step) {
        return true;
    }
    return this->step == other.step && this->type < other.type;
}

bool MessageTag::operator<=(MessageTag const& other) const
{
    return this->step <= other.step && this->type <= other.type;
}

bool MessageTag::operator>(MessageTag const& other) const
{
    if (this->step > other.step) {
        return true;
    }
    return this->step == other.step && this->type > other.type;
}

bool MessageTag::operator>=(MessageTag const& other) const
{
    return this->step >= other.step && this->type >= other.type;
}

void MessageTag::serialize(Serializer& serializer) const
{
    serializer << this->step << this->type;
}

void MessageTag::deserialize(Deserializer& deserializer)
{
    deserializer >> this->step >> this->type;
}

std::string MessageTag::to_string() const
{
    return std::string("MessageTag { step = ")
        + std::to_string(this->step)
        + ", type = "
        + std::to_string(this->type)
        + " }"
    ;
}


MessageHeader::MessageHeader() : seqn(0), timestamp(0)
{
}

void MessageHeader::fill_req()
{
    static std::atomic<uint64_t> current_seqn = 0;
    this->seqn = current_seqn.fetch_add(1);
    this->timestamp = time(NULL);
}

void MessageHeader::fill_resp(uint64_t seqn)
{
    this->seqn = seqn;
    this->timestamp = time(NULL);
}

void MessageHeader::serialize(Serializer& serializer) const
{
    serializer << this->seqn << this->timestamp;
}

void MessageHeader::deserialize(Deserializer& deserializer)
{
    deserializer >> this->seqn >> this->timestamp;
}

CastOnMessageError::CastOnMessageError(MessageError error) :
    error_(error),
    message("expected message to not be an error, but it is, code = ")
{
    this->message += std::to_string(error);
}

MessageError const& CastOnMessageError::error() const
{
    return this->error_;
}

const char *CastOnMessageError::what() const noexcept
{
    return this->message.c_str();
}

MessageBody::~MessageBody()
{
}

MessageErrorResp::MessageErrorResp() : MessageErrorResp(MSG_INTERNAL_ERR)
{
}

MessageErrorResp::MessageErrorResp(MessageError error) : error(error)
{
}

MessageTag MessageErrorResp::tag() const
{
    return MessageTag(MSG_RESP, MSG_ERROR);
}

void MessageErrorResp::serialize(Serializer& serializer) const
{
    serializer << (uint64_t) this->error;
}

void MessageErrorResp::deserialize(Deserializer& deserializer)
{
    uint64_t code;
    deserializer >> code;
    this->error = msg_error_from_code(code);
}


MessageClientConnReq::MessageClientConnReq()
{
}

MessageClientConnReq::MessageClientConnReq(Username const& username) :
    username(username)
{
}

MessageTag MessageClientConnReq::tag() const
{
    return MessageTag(MSG_REQ, MSG_CLIENT_CONN);
}

void MessageClientConnReq::serialize(Serializer& serializer) const
{
    serializer << this->username;
}

void MessageClientConnReq::deserialize(Deserializer& deserializer)
{
    deserializer >> this->username;
}

MessageTag MessageClientConnResp::tag() const
{
    return MessageTag(MSG_RESP, MSG_CLIENT_CONN);
}

void MessageClientConnResp::serialize(Serializer& serializer) const
{
}

void MessageClientConnResp::deserialize(Deserializer& deserializer)
{
}

MessageServerConnReq::MessageServerConnReq()
{
}

MessageTag MessageServerConnReq::tag() const
{
    return MessageTag(MSG_REQ, MSG_SERVER_CONN);
}

void MessageServerConnReq::serialize(Serializer& serializer) const
{
}

void MessageServerConnReq::deserialize(Deserializer& deserializer)
{
}

MessageTag MessageServerConnResp::tag() const
{
    return MessageTag(MSG_RESP, MSG_SERVER_CONN);
}

void MessageServerConnResp::serialize(Serializer& serializer) const
{
}

void MessageServerConnResp::deserialize(Deserializer& deserializer)
{
}

MessageTag MessageDisconnectReq::tag() const
{
    return MessageTag(MSG_REQ, MSG_DISCONNECT);
}

void MessageDisconnectReq::serialize(Serializer& serializer) const
{
}

void MessageDisconnectReq::deserialize(Deserializer& deserializer)
{
}

MessageTag MessageDisconnectResp::tag() const
{
    return MessageTag(MSG_RESP, MSG_DISCONNECT);
}

void MessageDisconnectResp::serialize(Serializer& serializer) const
{
}

void MessageDisconnectResp::deserialize(Deserializer& deserializer)
{
}

MessageTag MessagePingReq::tag() const
{
    return MessageTag(MSG_REQ, MSG_PING);
}

void MessagePingReq::serialize(Serializer& serializer) const
{
}

void MessagePingReq::deserialize(Deserializer& deserializer)
{
}

MessageTag MessagePingResp::tag() const
{
    return MessageTag(MSG_RESP, MSG_PING);
}

void MessagePingResp::serialize(Serializer& serializer) const
{
}

void MessagePingResp::deserialize(Deserializer& deserializer)
{
}

MessageFollowReq::MessageFollowReq()
{
}

MessageFollowReq::MessageFollowReq(Username const& username) :
    username(username)
{
}

MessageTag MessageFollowReq::tag() const
{
    return MessageTag(MSG_REQ, MSG_FOLLOW);
}

void MessageFollowReq::serialize(Serializer& serializer) const
{
    serializer << this->username;
}

void MessageFollowReq::deserialize(Deserializer& deserializer)
{
    deserializer >> this->username;
}

MessageTag MessageFollowResp::tag() const
{
    return MessageTag(MSG_RESP, MSG_FOLLOW);
}

void MessageFollowResp::serialize(Serializer& serializer) const
{
}

void MessageFollowResp::deserialize(Deserializer& deserializer)
{
}

MessageNotifyReq::MessageNotifyReq()
{
}

MessageNotifyReq::MessageNotifyReq(NotifMessage const& notif_msg) :
    notif_message(notif_msg)
{
}

MessageTag MessageNotifyReq::tag() const
{
    return MessageTag(MSG_REQ, MSG_NOTIFY);
}

void MessageNotifyReq::serialize(Serializer& serializer) const
{
    serializer << this->notif_message;
}

void MessageNotifyReq::deserialize(Deserializer& deserializer)
{
    deserializer >> this->notif_message;
}

MessageTag MessageNotifyResp::tag() const
{
    return MessageTag(MSG_RESP, MSG_NOTIFY);
}

void MessageNotifyResp::serialize(Serializer& serializer) const
{
}

void MessageNotifyResp::deserialize(Deserializer& deserializer)
{
}

MessageDeliverReq::MessageDeliverReq() : sent_at(0)
{
}

MessageDeliverReq::MessageDeliverReq(
    Username const& sender,
    NotifMessage const& notif_msg,
    int64_t sent_at
) :
    sender(sender),
    notif_message(notif_msg),
    sent_at(sent_at)
{
}

MessageTag MessageDeliverReq::tag() const
{
    return MessageTag(MSG_REQ, MSG_DELIVER);
}

void MessageDeliverReq::serialize(Serializer& serializer) const
{
    serializer << this->sender << this->notif_message << this->sent_at;
}

void MessageDeliverReq::deserialize(Deserializer& deserializer)
{
    deserializer >> this->sender >> this->notif_message >> this->sent_at;
}

MessageTag MessageDeliverResp::tag() const
{
    return MessageTag(MSG_RESP, MSG_DELIVER);
}

void MessageDeliverResp::serialize(Serializer& serializer) const
{
}

void MessageDeliverResp::deserialize(Deserializer& deserializer)
{
}

void Message::serialize(Serializer& serializer) const
{
    serializer
        << MSG_MAGIC_NUMBER
        << this->header
        << this->body->tag()
        << *this->body;
}

void Message::deserialize(Deserializer& deserializer)
{
    try {
        uint64_t maybe_magic_number;
        deserializer >> maybe_magic_number;
        if (maybe_magic_number != MSG_MAGIC_NUMBER) {
            throw MessageOutOfProtocol();
        }
    } catch (DeserializationUnexpectedEof const& exc) {
        throw MessageOutOfProtocol();
    }
    MessageTag tag;
    deserializer >> this->header >> tag;
    this->body.reset();
    switch (tag.step) {
        case MSG_REQ:
            switch (tag.type) {
                case MSG_CLIENT_CONN:
                    this->body =
                        std::shared_ptr<MessageBody>(new MessageClientConnReq);
                    break;
                case MSG_DISCONNECT:
                    this->body =
                        std::shared_ptr<MessageBody>(new MessageDisconnectReq);
                    break;
                case MSG_PING:
                    this->body =
                        std::shared_ptr<MessageBody>(new MessagePingReq);
                    break;
                case MSG_FOLLOW:
                    this->body =
                        std::shared_ptr<MessageBody>(new MessageFollowReq);
                    break;
                case MSG_NOTIFY:
                    this->body =
                        std::shared_ptr<MessageBody>(new MessageNotifyReq);
                    break;
                case MSG_DELIVER:
                    this->body =
                        std::shared_ptr<MessageBody>(new MessageDeliverReq);
                    break;
            }
            break;
        case MSG_RESP:
            switch (tag.type) {
                case MSG_ERROR:
                    this->body =
                        std::shared_ptr<MessageBody>(new MessageErrorResp);
                    break;
                case MSG_CLIENT_CONN:
                    this->body =
                        std::shared_ptr<MessageBody>(new MessageClientConnResp);
                    break;
                case MSG_DISCONNECT:
                    this->body =
                        std::shared_ptr<MessageBody>(new MessageDisconnectResp);
                    break;
                case MSG_PING:
                    this->body =
                        std::shared_ptr<MessageBody>(new MessagePingResp);
                    break;
                case MSG_FOLLOW:
                    this->body =
                        std::shared_ptr<MessageBody>(new MessageFollowResp);
                    break;
                case MSG_NOTIFY:
                    this->body =
                        std::shared_ptr<MessageBody>(new MessageNotifyResp);
                    break;
                case MSG_DELIVER:
                    this->body =
                        std::shared_ptr<MessageBody>(new MessageDeliverResp);
                    break;
            }
            break;
    }
    if (!this->body) {
        throw InvalidMessagePayload(
            std::string("invalid message tag:  ") + tag.to_string()
        );
    }
    deserializer >> *this->body;
}

Enveloped::Enveloped()
{
}

Enveloped::Enveloped(Address remote, Message message) :
    remote(remote),
    message(message)
{
}
