#include "shared.h"
#include "../shared/address.h"
#include "../shared/message.h"

static TestSuite parse_udp_port_test_suite();
static TestSuite serializer_test_suite();
static TestSuite deserializer_test_suite();

TestSuite shared_test_suite()
{
    return TestSuite()
        .append(parse_udp_port_test_suite())
        .append(serializer_test_suite())
        .append(deserializer_test_suite())
    ;
}

static TestSuite parse_udp_port_test_suite()
{
    return TestSuite()
        .test("parse_udp_port min", [] {
            uint16_t actual = parse_udp_port("1");
            TEST_ASSERT(
                std::string("found ") + std::to_string(actual),
                actual == 1
            );
        })

        .test("parse_udp_port max", [] {
            uint16_t actual = parse_udp_port("65535");
            TEST_ASSERT(
                std::string("found ") + std::to_string(actual),
                actual == 65535
            );
        })

        .test("parse_udp_port below min", [] {
            bool throwed = false;
            uint16_t actual = 0;
            try {
                actual = parse_udp_port("0");
            } catch (InvalidUdpPort const &exception) {
                throwed = true;
            }
            TEST_ASSERT(
                std::string("should throw, but found") + std::to_string(actual),
                throwed
            );
        })

        .test("parse_udp_port above max", [] {
            bool throwed = false;
            uint16_t actual = 0;
            try {
                actual = parse_udp_port("65536");
            } catch (InvalidUdpPort const &exception) {
                throwed = true;
            }
            TEST_ASSERT(
                std::string("should throw, but found") + std::to_string(actual),
                throwed
            );
        })

        .test("parse_udp_port bad chars", [] {
            bool throwed = false;
            uint16_t actual = 0;
            try {
                actual = parse_udp_port("1s3");
            } catch (InvalidUdpPort const &exception) {
                throwed = true;
            }
            TEST_ASSERT(
                std::string("should throw, but found") + std::to_string(actual),
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
            serializer << (int64_t) -79 << (uint64_t) 143 << "The End";
            std::string actual = serializer.finish();
            TEST_ASSERT(
                std::string("found ") + actual,
                actual == "-79;143;The End;"
            );
        })

        .test("serialize strings with various escapes", [] {
            MessageSerializer serializer;
            serializer
                << "The End"
                << "The; End"
                << "The\\ End"
                << "The\\\\ End"
                << "The\\; End";
            std::string actual = serializer.finish();
            char const *expected =
                "The End;The\\; End;The\\\\ End;The\\\\\\\\ End;The\\\\\\; End;"
            ;
            TEST_ASSERT(
                std::string("found ") + actual,
                actual == expected
            );
        })
    ;
}

static TestSuite deserializer_test_suite()
{
    return TestSuite()
        .test("deserialize fields of all types", [] {
            std::string message = "-79;143;The End;" ;
            MessageDeserializer deserializer(message);

            int64_t int_field;
            uint64_t uint_field;
            std::string text_field;

            deserializer >> int_field >> uint_field >> text_field;

            TEST_ASSERT(
                std::string("int field, found: ") + std::to_string(int_field),
                int_field == -79
            );
            TEST_ASSERT(
                std::string("uint field, found: ") + std::to_string(uint_field),
                uint_field == 143
            );
            TEST_ASSERT(
                std::string("text field, found: ") + text_field,
                text_field == "The End"
            );
        })

        .test("deserialize strings with various escapes", [] {
            std::string message =
                "The End;The\\; End;The\\\\ End;The\\\\\\\\ End;The\\\\\\; End;"
            ;
            MessageDeserializer deserializer(message);

            std::string fields[5];

            deserializer
                >> fields[0]
                >> fields[1]
                >> fields[2]
                >> fields[3]
                >> fields[4];

            TEST_ASSERT(
                std::string("field 0, found: ") + fields[0],
                fields[0] == "The End" 
            );
            TEST_ASSERT(
                std::string("field 1, found: ") + fields[1],
                fields[1] == "The; End" 
            );
            TEST_ASSERT(
                std::string("field 2, found: ") + fields[2],
                fields[2] == "The\\ End" 
            );
            TEST_ASSERT(
                std::string("field 3, found: ") + fields[3],
                fields[3] == "The\\\\ End" 
            );
            TEST_ASSERT(
                std::string("field 4, found: ") + fields[4],
                fields[4] == "The\\; End" 
            );
        })
    ;
}
