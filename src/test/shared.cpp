#include "shared.h"
#include "../shared/address.h"
#include "../shared/message.h"

static TestSuite parse_udp_port_test_suite();
static TestSuite serializer_test_suite();

TestSuite shared_test_suite()
{
    return TestSuite()
        .append(parse_udp_port_test_suite())
        .append(serializer_test_suite())
    ;
}

static TestSuite parse_udp_port_test_suite()
{
    return TestSuite()
        .test("parse_udp_port min", [] {
            TEST_ASSERT(
                "parse port 1 should be ok",
                parse_udp_port("1") == 1
            );
        })
        .test("parse_udp_port max", [] {
            TEST_ASSERT(
                "parse port 65535 should be ok",
                parse_udp_port("65535") == 65535
            );
        })
        .test("parse_udp_port below min", [] {
            bool throwed = false;
            try {
                parse_udp_port("0");
            } catch (InvalidUdpPort const &exception) {
                throwed = true;
            }
            TEST_ASSERT(
                "parse port 0 should be an error",
                throwed
            );
        })
        .test("parse_udp_port above max", [] {
            bool throwed = false;
            try {
                parse_udp_port("65536");
            } catch (InvalidUdpPort const &exception) {
                throwed = true;
            }
            TEST_ASSERT(
                "parse port 65536 should be an error",
                throwed
            );
        })
        .test("parse_udp_port bad chars", [] {
            bool throwed = false;
            try {
                parse_udp_port("1s3");
            } catch (InvalidUdpPort const &exception) {
                throwed = true;
            }
            TEST_ASSERT(
                "parse port with 's' should be an error",
                throwed
            );
        })
    ;
}

static TestSuite serializer_test_suite()
{
    return TestSuite()
        .test("serialize fields of all types", [] {
            MessageSerializer serializer;
            serializer << (int64_t) -79;
            serializer << (uint64_t) 143;
            serializer << "The End";
            std::vector<uint8_t> bytes = serializer.finish();
            std::string text(bytes.begin(), bytes.end());
            TEST_ASSERT(
                std::string("found ") + text,
                text == "-79;143;The End;"
            );
        })
    ;
}
