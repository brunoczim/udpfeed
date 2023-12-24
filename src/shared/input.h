#ifndef SHARED_INPUT_H_
#define SHARED_INPUT_H_ 1

class TermIoError : public SocketError {
    private:
        std::string full_message;
        int c_errno_;
        std::string message;

    public:
        TermIoError(std::string const& message);
        int c_errno() const;
        virtual const char *what() const noexcept;
};

class TerminalInput {
    private:
        static std::once_flag init_flag;

    public:
        static TerminalInput instance;

        size_t read(uint8_t *buf, size_t capacity);

        std::optional<uint8_t> getc();
};

extern TerminalInput term_input;

#endif
