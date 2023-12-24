#include "data.h"

ClientNotifNotice::ClientNotifNotice(
    Username const& sender,
    NotifMessage const& message,
    int64_t sent_at
) :
    sender(sender),
    message(message),
    sent_at(sent_at)
{
}

ClientOutputNotice::Type ClientNotifNotice::type() const
{
    return ClientOutputNotice::NOTIF;
}

ClientErrorNotice::ClientErrorNotice(MessageError error) : error(error)
{
}

ClientOutputNotice::Type ClientErrorNotice::type() const
{
    return ClientOutputNotice::ERROR;
}

ClientFollowCommand::ClientFollowCommand(Username const& username) :
    username(username)
{
}

ClientInputCommand::Type ClientFollowCommand::type() const
{
    return ClientInputCommand::FOLLOW;
}

ClientSendCommand::ClientSendCommand(NotifMessage const& message) :
    message(message)
{
}

ClientInputCommand::Type ClientSendCommand::type() const
{
    return ClientInputCommand::SEND;
}
