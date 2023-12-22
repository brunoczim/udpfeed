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
            table.notify(
                Address(make_ipv4({ 127, 0, 0, 1 }), 3232),
                NotifMessage("Hello, World!"),
                3
            );
            std::optional<PendingNotif> pending_notif = table.consume_one_notif(
                Username("@goodbye")
            );
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
        })
    ;
}
