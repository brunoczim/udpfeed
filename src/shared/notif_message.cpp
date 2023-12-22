#include "notif_message.h"

char const *UninitializedNotifMessage::what() const noexcept
{
    return "accessed uninitalized notification message";
}

InvalidNotifMessage::InvalidNotifMessage(
    std::string const& content,
    std::string const& why
) :
    DeserializationError("\""),
    content_(content)
{
    this->message += content;
    this->message += "\": ";
    this->message += why;
}

std::string const& InvalidNotifMessage::content() const
{
    return this->content_;
}

NotifMessage::NotifMessage()
{
}

NotifMessage::NotifMessage(std::string const& content)
{
    if (content.size() < NotifMessage::MIN_LEN) {
        throw InvalidNotifMessage(content, "notification is too short");
    }
    if (content.size() > NotifMessage::MAX_LEN) {
        throw InvalidNotifMessage(content, "notification is too long");
    }

    this->content_ = std::shared_ptr<std::string>(new std::string(content));
}

std::string const& NotifMessage::content() const
{
    if (!this->content_) {
        throw UninitializedNotifMessage();
    }
    return *this->content_;
}

bool NotifMessage::operator==(NotifMessage const& other) const
{
    return this->content() == other.content();
}

bool NotifMessage::operator!=(NotifMessage const& other) const
{
    return this->content() != other.content();
}

bool NotifMessage::operator<(NotifMessage const& other) const
{
    return this->content() < other.content();
}

bool NotifMessage::operator<=(NotifMessage const& other) const
{
    return this->content() <= other.content();
}

bool NotifMessage::operator>(NotifMessage const& other) const
{
    return this->content() > other.content();
}

bool NotifMessage::operator>=(NotifMessage const& other) const
{
    return this->content() >= other.content();
}

void NotifMessage::serialize(Serializer& serializer) const
{
    serializer << this->content();
}

void NotifMessage::deserialize(Deserializer& deserializer)
{
    std::string content;
    deserializer >> content;
    NotifMessage username(content);
    *this = std::move(username);
}
