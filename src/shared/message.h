#ifndef SHARED_MESSAGE_H_
#define SHARED_MESSAGE_H_ 1

#include <cstdint>
#include <string>

class InvalidMessagePayload: public std::exception {
    private:
        std::string error_message;
    public:
        InvalidMessagePayload(std::string const& error_message);

        virtual const char *what() const noexcept;
};

class MessageSerializer {
    private:
        std::string bytes;
    public:
        std::string const& finish() const;

        MessageSerializer& operator<<(char const *data);

        MessageSerializer& operator<<(std::string const& data);

        MessageSerializer& operator<<(uint64_t data);

        MessageSerializer& operator<<(int64_t data);
};

class MessageDeserializer {
    private:
        std::string const& bytes;
        size_t i;
    public:
        MessageDeserializer(std::string const& bytes);

        MessageDeserializer& operator>>(std::string &data);

        MessageDeserializer& operator>>(uint64_t &data);

        MessageDeserializer& operator>>(int64_t &data);

    private:
        uint8_t next();
};

class MessagePayload {
    virtual void serialize(MessageSerializer& MessageSerializer) const = 0;

    virtual void deserialize(MessageDeserializer& MessageDeserializer) = 0;

    virtual ~MessagePayload();
};

#endif