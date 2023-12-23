#ifndef CLIENT_DATA_H_
#define CLIENT_DATA_H_ 1

#include "../shared/username.h"
#include "../shared/notif_message.h"
#include "../shared/message.h"

class ClientOutputNotice {
    public:
        enum Type {
            NOTIF,
            ERROR
        };
  
        virtual Type type() const = 0;
};

class ClientNotifNotice : public ClientOutputNotice {
    public:
        Username sender;
        NotifMessage message;

        ClientNotifNotice(Username const& sender, NotifMessage const& message);

        virtual ClientOutputNotice::Type type() const;
};

class ClientErrorNotice : public ClientOutputNotice {
    public:
        MessageError error;

        ClientErrorNotice(MessageError error);

        virtual ClientOutputNotice::Type type() const;
};

class ClientInputCommand {
    public:
        enum Type {
            FOLLOW,
            SEND
        };

        virtual Type type() const = 0;
};

class ClientFollowCommand : public ClientInputCommand {
    public:
        Username username;
        
        ClientFollowCommand(Username const& username);

        virtual ClientInputCommand::Type type() const;
};

class ClientSendCommand : public ClientInputCommand {
    public:
        NotifMessage message;

        ClientSendCommand(NotifMessage const& message);

        virtual ClientInputCommand::Type type() const;
};

#endif
