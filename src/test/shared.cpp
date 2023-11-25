#include "shared.h"
#include "../shared/address.h"

TestSuite shared_test_suite()
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
