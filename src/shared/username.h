#ifndef SHARED_USERNAME_H_
#define SHARED_USERNAME_H_ 1

#include <exception>
#include <string>
#include <memory>
#include "serialization.h"

class UninitializedUsername : public std::exception {
    public:
        virtual char const *what() const noexcept;
};

class InvalidUsername : public std::exception {
    private:
        std::string content_;
        std::string message;

    public:
        InvalidUsername(std::string const& content, std::string const& why);

        std::string const& content() const;

        virtual char const *what() const noexcept;
};

class Username : public Serializable, public Deserializable {
    private:
        std::shared_ptr<std::string> content_;
    public:
        static constexpr size_t MIN_LEN = 4;
        static constexpr size_t MAX_LEN = 20;

        Username();
        Username(std::string const& content);

        std::string const& content() const;

        bool operator==(Username const& other) const;
        bool operator!=(Username const& other) const;

        bool operator<(Username const& other) const;
        bool operator<=(Username const& other) const;
        bool operator>(Username const& other) const;
        bool operator>=(Username const& other) const;

        virtual void serialize(Serializer& serializer) const;
        virtual void deserialize(Deserializer& deserializer);
};

#endif
