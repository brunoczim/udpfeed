#include "server.h"
#include "../server/data.h"

static TestSuite server_data_test_suite();

TestSuite server_test_suite()
{
    return TestSuite()
        .append(server_data_test_suite())
    ;
}

static TestSuite server_data_test_suite()
{
    return TestSuite()
        .test("just connect", [] {
            ServerProfileTable table;
            table.connect(
                Address(make_ipv4({ 127, 0, 0, 1 }), 3232),
                Username("@helloworld"),
                0
            );
        })
        .test("just disconnect", [] {
            ServerProfileTable table;
            table.connect(
                Address(make_ipv4({ 127, 0, 0, 1 }), 3232),
                Username("@helloworld"),
                0
            );
            table.disconnect(Address(make_ipv4({ 127, 0, 0, 1 }), 3232), 1);
        })
        .test("just follow", [] {
            ServerProfileTable table;
            table.connect(
                Address(make_ipv4({ 127, 0, 0, 1 }), 3232),
                Username("@helloworld"),
                0
            );
            table.connect(
                Address(make_ipv4({ 127, 0, 0, 1 }), 4545),
                Username("@goodbye"),
                1
            );
            table.follow(
                Address(make_ipv4({ 127, 0, 0, 1 }), 4545),
                Username("@helloworld"),
                2
            );
        })
        .test("just notify", [] {
            ServerProfileTable table;
            table.connect(
                Address(make_ipv4({ 127, 0, 0, 1 }), 3232),
                Username("@helloworld"),
                0
            );
            table.connect(
                Address(make_ipv4({ 127, 0, 0, 1 }), 4545),
                Username("@goodbye"),
                1
            );
            table.follow(
                Address(make_ipv4({ 127, 0, 0, 1 }), 4545),
                Username("@helloworld"),
                2
            );

            Channel<Username> channel;
            table.notify(
                Address(make_ipv4({ 127, 0, 0, 1 }), 3232),
                NotifMessage("Hello, World!"),
                channel.sender,
                3
            );

            Username follower_username = channel.receiver.receive();
            TEST_ASSERT(
                "follower user name should be @goodbye, found"
                    + follower_username.content(),
                follower_username == Username("@goodbye")
            );

            std::optional<PendingNotif> pending_notif;

            pending_notif = table.consume_one_notif(Username("@goodbye"));
            TEST_ASSERT(
                "optional should not be null",
                pending_notif.has_value()
            );
            TEST_ASSERT(
                std::string("sender should be @helloworld, found ")
                    + pending_notif->sender.content(),
                pending_notif->sender == Username("@helloworld")
            );
            TEST_ASSERT(
                std::string("notif message found ")
                    + pending_notif->message.content(),
                pending_notif->message == NotifMessage("Hello, World!")
            );
            TEST_ASSERT(
                std::string("address count is ")
                    + std::to_string(pending_notif->receivers.size()),
                pending_notif->receivers == std::set {
                    Address(make_ipv4({ 127, 0, 0, 1 }), 4545)
                }
            );

            pending_notif = table.consume_one_notif(Username("@goodbye"));
            TEST_ASSERT(
                "optional should be null",
                !pending_notif.has_value()
            );
        })

        .test("full database workflow", [] {
            ServerProfileTable table;
            table.connect(
                Address(make_ipv4({ 127, 0, 0, 1 }), 3232),
                Username("@helloworld"),
                0
            );
            table.connect(
                Address(make_ipv4({ 127, 0, 0, 1 }), 4545),
                Username("@goodbye"),
                1
            );
            table.connect(
                Address(make_ipv4({ 127, 0, 0, 1 }), 6665),
                Username("@bruno"),
                2
            );
            table.connect(
                Address(make_ipv4({ 127, 0, 0, 1 }), 6667),
                Username("@cavejohnson"),
                3
            );
            table.connect(
                Address(make_ipv4({ 127, 0, 0, 1 }), 7878),
                Username("@whoo"),
                4
            );
            table.follow(
                Address(make_ipv4({ 127, 0, 0, 1 }), 4545),
                Username("@helloworld"),
                5
            );
            table.follow(
                Address(make_ipv4({ 127, 0, 0, 1 }), 6667),
                Username("@helloworld"),
                6
            );
            table.follow(
                Address(make_ipv4({ 127, 0, 0, 1 }), 6665),
                Username("@helloworld"),
                7
            );
            table.follow(
                Address(make_ipv4({ 127, 0, 0, 1 }), 6665),
                Username("@cavejohnson"),
                8
            );

            Channel<Username> channel;

            table.notify(
                Address(make_ipv4({ 127, 0, 0, 1 }), 3232),
                NotifMessage("Hello"),
                channel.sender,
                9
            );
            table.notify(
                Address(make_ipv4({ 127, 0, 0, 1 }), 3232),
                NotifMessage("Bye"),
                channel.sender,
                10 
            );
            table.notify(
                Address(make_ipv4({ 127, 0, 0, 1 }), 6667),
                NotifMessage("...wait"),
                channel.sender,
                11 
            );
            table.disconnect(
                Address(make_ipv4({ 127, 0, 0, 1 }), 6665),
                12 
            );
            table.notify(
                Address(make_ipv4({ 127, 0, 0, 1 }), 3232),
                NotifMessage("Bye Bye Bye"),
                channel.sender,
                13 
            );

            std::optional<PendingNotif> pending_notif;

            pending_notif = table.consume_one_notif(Username("@goodbye"));
            TEST_ASSERT(
                "optional should not be null",
                pending_notif.has_value()
            );
            TEST_ASSERT(
                std::string("sender should be @helloworld, found ")
                    + pending_notif->sender.content(),
                pending_notif->sender == Username("@helloworld")
            );
            TEST_ASSERT(
                std::string("notif message found ")
                    + pending_notif->message.content(),
                pending_notif->message == NotifMessage("Hello")
            );

            pending_notif = table.consume_one_notif(Username("@goodbye"));
            TEST_ASSERT(
                "optional should not be null",
                pending_notif.has_value()
            );
            TEST_ASSERT(
                std::string("sender should be @helloworld, found ")
                    + pending_notif->sender.content(),
                pending_notif->sender == Username("@helloworld")
            );
            TEST_ASSERT(
                std::string("notif message found ")
                    + pending_notif->message.content(),
                pending_notif->message == NotifMessage("Bye")
            );

            pending_notif = table.consume_one_notif(Username("@goodbye"));
            TEST_ASSERT(
                "optional should not be null",
                pending_notif.has_value()
            );
            TEST_ASSERT(
                std::string("sender should be @helloworld, found ")
                    + pending_notif->sender.content(),
                pending_notif->sender == Username("@helloworld")
            );
            TEST_ASSERT(
                std::string("notif message found ")
                    + pending_notif->message.content(),
                pending_notif->message == NotifMessage("Bye Bye Bye")
            );

            pending_notif = table.consume_one_notif(Username("@goodbye"));
            TEST_ASSERT(
                "optional should be null",
                !pending_notif.has_value()
            );

            pending_notif = table.consume_one_notif(Username("@cavejohnson"));
            TEST_ASSERT(
                "optional should not be null",
                pending_notif.has_value()
            );
            TEST_ASSERT(
                std::string("sender should be @helloworld, found ")
                    + pending_notif->sender.content(),
                pending_notif->sender == Username("@helloworld")
            );
            TEST_ASSERT(
                std::string("notif message found ")
                    + pending_notif->message.content(),
                pending_notif->message == NotifMessage("Hello")
            );

            pending_notif = table.consume_one_notif(Username("@cavejohnson"));
            TEST_ASSERT(
                "optional should not be null",
                pending_notif.has_value()
            );
            TEST_ASSERT(
                std::string("sender should be @helloworld, found ")
                    + pending_notif->sender.content(),
                pending_notif->sender == Username("@helloworld")
            );
            TEST_ASSERT(
                std::string("notif message found ")
                    + pending_notif->message.content(),
                pending_notif->message == NotifMessage("Bye")
            );

            pending_notif = table.consume_one_notif(Username("@cavejohnson"));
            TEST_ASSERT(
                "optional should not be null",
                pending_notif.has_value()
            );
            TEST_ASSERT(
                std::string("sender should be @helloworld, found ")
                    + pending_notif->sender.content(),
                pending_notif->sender == Username("@helloworld")
            );
            TEST_ASSERT(
                std::string("notif message found ")
                    + pending_notif->message.content(),
                pending_notif->message == NotifMessage("Bye Bye Bye")
            );

            pending_notif = table.consume_one_notif(Username("@cavejohnson"));
            TEST_ASSERT(
                "optional should be null",
                !pending_notif.has_value()
            );

            pending_notif = table.consume_one_notif(Username("@bruno"));
            TEST_ASSERT(
                "optional should not be null",
                pending_notif.has_value()
            );
            TEST_ASSERT(
                std::string("sender should be @helloworld, found ")
                    + pending_notif->sender.content(),
                pending_notif->sender == Username("@helloworld")
            );
            TEST_ASSERT(
                std::string("notif message found ")
                    + pending_notif->message.content(),
                pending_notif->message == NotifMessage("Hello")
            );

            pending_notif = table.consume_one_notif(Username("@bruno"));
            TEST_ASSERT(
                "optional should not be null",
                pending_notif.has_value()
            );
            TEST_ASSERT(
                std::string("sender should be @helloworld, found ")
                    + pending_notif->sender.content(),
                pending_notif->sender == Username("@helloworld")
            );
            TEST_ASSERT(
                std::string("notif message found ")
                    + pending_notif->message.content(),
                pending_notif->message == NotifMessage("Bye")
            );

            pending_notif = table.consume_one_notif(Username("@bruno"));
            TEST_ASSERT(
                "optional should not be null",
                pending_notif.has_value()
            );
            TEST_ASSERT(
                std::string("sender should be @cavejohnson, found ")
                    + pending_notif->sender.content(),
                pending_notif->sender == Username("@cavejohnson")
            );
            TEST_ASSERT(
                std::string("notif message found ")
                    + pending_notif->message.content(),
                pending_notif->message == NotifMessage("...wait")
            );

            pending_notif = table.consume_one_notif(Username("@bruno"));
            TEST_ASSERT(
                "optional should not be null",
                pending_notif.has_value()
            );
            TEST_ASSERT(
                std::string("sender should be @helloworld, found ")
                    + pending_notif->sender.content(),
                pending_notif->sender == Username("@helloworld")
            );
            TEST_ASSERT(
                std::string("notif message found ")
                        + pending_notif->message.content(),
                pending_notif->message == NotifMessage("Bye Bye Bye")
            );

            pending_notif = table.consume_one_notif(Username("@bruno"));
            TEST_ASSERT(
                "optional should be null",
                !pending_notif.has_value()
            );
            
            pending_notif = table.consume_one_notif(Username("@whoo"));
            TEST_ASSERT(
                "optional should be null",
                !pending_notif.has_value()
            );

            table.disconnect(
                Address(make_ipv4({ 127, 0, 0, 1 }), 3232),
                14 
            );

            table.disconnect(
                Address(make_ipv4({ 127, 0, 0, 1 }), 6667),
                15 
            );

            table.disconnect(
                Address(make_ipv4({ 127, 0, 0, 1 }), 7878),
                16 
            );

            table.disconnect(
                Address(make_ipv4({ 127, 0, 0, 1 }), 4545),
                17 
            );
        })

        .test("do not allow three sessions", [] {
            ServerProfileTable table;
            table.connect(
                Address(make_ipv4({ 127, 0, 0, 1 }), 3232),
                Username("@helloworld"),
                0
            );
            table.connect(
                Address(make_ipv4({ 127, 0, 0, 1 }), 4545),
                Username("@goodbye"),
                1
            );
            table.connect(
                Address(make_ipv4({ 127, 0, 0, 1 }), 7878),
                Username("@helloworld"),
                0
            );

            std::optional<MessageError> error;
            try {
                table.connect(
                    Address(make_ipv4({ 127, 0, 0, 1 }), 7878),
                    Username("@helloworld"),
                    0
                );
            } catch (ThrowableMessageError const& exc) {
                error = std::make_optional(exc.error());
            }

            TEST_ASSERT(
                "exception should have thrown but didn't",
                error.has_value()
            );

            TEST_ASSERT(
                std::string("expected MSG_TOO_MANY_SESSIONS, found: ")
                    + std::to_string(*error),
                *error == MSG_TOO_MANY_SESSIONS
            );
        })

        .test("do not follow without connect", [] {
            ServerProfileTable table;
            table.connect(
                Address(make_ipv4({ 127, 0, 0, 1 }), 3232),
                Username("@helloworld"),
                0
            );

            std::optional<MessageError> error;
            try {
                table.follow(
                    Address(make_ipv4({ 127, 0, 0, 1 }), 4545),
                    Username("@helloworld"),
                    1
                );
            } catch (ThrowableMessageError const& exc) {
                error = std::make_optional(exc.error());
            }

            TEST_ASSERT(
                "exception should have thrown but didn't",
                error.has_value()
            );

            TEST_ASSERT(
                std::string("expected MSG_NO_CONNECTION, found: ")
                    + std::to_string(*error),
                *error == MSG_NO_CONNECTION
            );
        })

        .test("do not notify without connect", [] {
            ServerProfileTable table;
            std::optional<MessageError> error;
            try {
                Channel<Username> channel;
                table.notify(
                    Address(make_ipv4({ 127, 0, 0, 1 }), 4545),
                    NotifMessage("blablabla"),
                    channel.sender,
                    0
                );
            } catch (ThrowableMessageError const& exc) {
                error = std::make_optional(exc.error());
            }

            TEST_ASSERT(
                "exception should have thrown but didn't",
                error.has_value()
            );

            TEST_ASSERT(
                std::string("expected MSG_NO_CONNECTION, found: ")
                    + std::to_string(*error),
                *error == MSG_NO_CONNECTION
            );
        })

        .test("do not follow unknown profile", [] {
            ServerProfileTable table;
            table.connect(
                Address(make_ipv4({ 127, 0, 0, 1 }), 3232),
                Username("@helloworld"),
                0
            );
            table.connect(
                Address(make_ipv4({ 127, 0, 0, 1 }), 4545),
                Username("@goodbye"),
                1
            );

            std::optional<MessageError> error;
            try {
                table.follow(
                    Address(make_ipv4({ 127, 0, 0, 1 }), 4545),
                    Username("@void"),
                    2
                );
            } catch (ThrowableMessageError const& exc) {
                error = std::make_optional(exc.error());
            }

            TEST_ASSERT(
                "exception should have thrown but didn't",
                error.has_value()
            );

            TEST_ASSERT(
                std::string("expected MSG_UNKNOWN_USERNAME, found: ")
                    + std::to_string(*error),
                *error == MSG_UNKNOWN_USERNAME
            );
        })
    ;
}
