#ifndef SHARED_NOTIF_MESSAGE_H_
#define SHARED_NOTIF_MESSAGE_H_ 1

#include <exception>
#include <string>
#include <memory>
#include "serialization.h"

class UninitializedNotifMessage : public std::exception {
    public:
        virtual char const *what() const noexcept;
};

class InvalidNotifMessage : public std::exception {
    private:
        std::string content_;
        std::string message;

    public:
        InvalidNotifMessage(std::string const& content, std::string const& why);

        std::string const& content() const;

        virtual char const *what() const noexcept;
};

class NotifMessage : public Serializable, public Deserializable {
    private:
        std::shared_ptr<std::string> content_;
    public:
        static constexpr size_t MIN_LEN = 1;
        static constexpr size_t MAX_LEN = 128;

        NotifMessage();
        NotifMessage(std::string const& content);

        std::string const& content() const;

        bool operator==(NotifMessage const& other) const;
        bool operator!=(NotifMessage const& other) const;

        bool operator<(NotifMessage const& other) const;
        bool operator<=(NotifMessage const& other) const;
        bool operator>(NotifMessage const& other) const;
        bool operator>=(NotifMessage const& other) const;

        virtual void serialize(Serializer& serializer) const;
        virtual void deserialize(Deserializer& deserializer);
};

#endif
