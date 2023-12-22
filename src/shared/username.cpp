#include "username.h"

#include <cctype>

char const *UninitializedUsername::what() const noexcept
{
    return "accessed uninitalized username";
}

InvalidUsername::InvalidUsername(
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

std::string const& InvalidUsername::content() const
{
    return this->content_;
}

Username::Username()
{
}

Username::Username(std::string const& content)
{
    if (content.size() < Username::MIN_LEN + 1) {
        throw InvalidUsername(content, "usernames is too short");
    }
    if (content.size() > Username::MAX_LEN + 1) {
        throw InvalidUsername(content, "username is too long");
    }
    if (content[0] != '@') {
        throw InvalidUsername(content, "usernames must be prefixed with '@'");
    }
    if (!std::isalpha(content[1]) && content[1] != '_') {
        throw InvalidUsername(
            content,
            "first username character must be ASCII letter or underscore"
        );
    }

    for (size_t i = 2; i < content.size(); i++) {
        if (
            !std::isalpha(content[i])
            && !std::isdigit(content[i])
            && content[i] != '_'
        ) {
            throw InvalidUsername(
                content,
                "username characters must be ASCII letter, digits or underscore"
            );
        }
    }

    this->content_ = std::shared_ptr<std::string>(new std::string(content));
}

std::string const& Username::content() const
{
    if (!this->content_) {
        throw UninitializedUsername();
    }
    return *this->content_;
}

bool Username::operator==(Username const& other) const
{
    return this->content() == other.content();
}

bool Username::operator!=(Username const& other) const
{
    return this->content() != other.content();
}

bool Username::operator<(Username const& other) const
{
    return this->content() < other.content();
}

bool Username::operator<=(Username const& other) const
{
    return this->content() <= other.content();
}

bool Username::operator>(Username const& other) const
{
    return this->content() > other.content();
}

bool Username::operator>=(Username const& other) const
{
    return this->content() >= other.content();
}

void Username::serialize(Serializer& serializer) const
{
    serializer << this->content();
}

void Username::deserialize(Deserializer& deserializer)
{
    std::string content;
    deserializer >> content;
    Username username(content);
    *this = std::move(username);
}
