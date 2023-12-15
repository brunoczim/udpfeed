#include <set>
#include <thread>
#include "shared.h"
#include "../shared/address.h"
#include "../shared/message.h"
#include "../shared/socket.h"
#include "../shared/channel.h"

static TestSuite parse_udp_port_test_suite();
static TestSuite plaintext_ser_test_suite();
static TestSuite plaintext_de_test_suite();
static TestSuite socket_test_suite();
static TestSuite channel_test_suite();

TestSuite shared_test_suite()
{
    return TestSuite()
        .append(parse_udp_port_test_suite())
        .append(plaintext_ser_test_suite())
        .append(plaintext_de_test_suite())
        .append(socket_test_suite())
        .append(channel_test_suite())
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
                std::string("should throw, but found ") + std::to_string(actual),
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
                std::string("should throw, but found ") + std::to_string(actual),
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
                std::string("should throw, but found ") + std::to_string(actual),
                throwed
            );
        })
    ;
}

static TestSuite plaintext_ser_test_suite()
{
    return TestSuite()
        .test("serialize fields of all types", [] {
            std::ostringstream ostream;
            PlaintextSerializer serializer_impl(ostream);
            Serializer& serializer = serializer_impl;
            serializer
                << (uint8_t) 138
                << false
                << (uint16_t) 1243
                << (uint32_t) 78679
                << (uint64_t) 143
                << (int8_t) -14
                << "The End"
                << (int16_t) -8430
                << (int32_t) -32
                << std::vector<int32_t> { -1, 3 }
                << (int64_t) -79;
            std::string actual = ostream.str();
            char const *expected =
                "138;0;1243;78679;143;-14;The End;-8430;-32;2;-1;3;-79;";
            TEST_ASSERT(
                std::string("found ") + actual,
                actual == expected
            );
        })

        .test("serialize strings with various escapes", [] {
            std::ostringstream ostream;
            PlaintextSerializer serializer_impl(ostream);
            Serializer& serializer = serializer_impl;
            serializer
                << "The End"
                << "The; End"
                << "The\\ End"
                << "The\\\\ End"
                << "The\\; End";
            std::string actual = ostream.str();
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


static TestSuite plaintext_de_test_suite()
{
    return TestSuite()
        .test("deserialize fields of all types", [] {
            std::istringstream istream(
                "1;138;1243;78679;143;-14;-8430;-32;The End;2;-1;3;-79;"
            );
            PlaintextDeserializer deserializer_impl(istream);
            Deserializer& deserializer = deserializer_impl;

            bool bool_field;
            uint8_t u8_field;
            uint16_t u16_field;
            uint32_t u32_field;
            uint64_t u64_field;
            int8_t i8_field;
            int16_t i16_field;
            int32_t i32_field;
            std::string text_field;
            std::vector<int16_t> vec_field;
            int64_t i64_field;

            deserializer
                >> bool_field
                >> u8_field
                >> u16_field
                >> u32_field
                >> u64_field
                >> i8_field
                >> i16_field
                >> i32_field
                >> text_field
                >> vec_field
                >> i64_field;

            TEST_ASSERT(
                std::string("bool field, found ") + std::to_string(bool_field),
                bool_field
            );

            TEST_ASSERT(
                std::string("uint8 field, found ") + std::to_string(u8_field),
                u8_field == 138
            );
            TEST_ASSERT(
                std::string("uint16 field, found ") + std::to_string(u16_field),
                u16_field == 1243
            );
            TEST_ASSERT(
                std::string("uint32 field, found ") + std::to_string(u32_field),
                u32_field == 78679
            );
            TEST_ASSERT(
                std::string("uint64 field, found ") + std::to_string(u64_field),
                u64_field == 143
            );
            TEST_ASSERT(
                std::string("int8 field, found ") + std::to_string(i8_field),
                i8_field == -14
            );
            TEST_ASSERT(
                std::string("int16 field, found ") + std::to_string(i16_field),
                i16_field == -8430
            );
            TEST_ASSERT(
                std::string("int32 field, found ") + std::to_string(i32_field),
                i32_field == -32
            );
            TEST_ASSERT(
                std::string("text field, found ") + text_field,
                text_field == "The End"
            );
            TEST_ASSERT(
                std::string("vector field length, found ")
                    + std::to_string(vec_field.size()),
                vec_field.size() == 2
            );
            TEST_ASSERT(
                std::string("vector field @ 0, found ")
                    + std::to_string(vec_field[0]),
                vec_field[0] == -1
            );
            TEST_ASSERT(
                std::string("vector field @ 1, found ")
                    + std::to_string(vec_field[1]),
                vec_field[1] == 3
            );
            TEST_ASSERT(
                std::string("int64 field, found ") + std::to_string(i64_field),
                i64_field == -79
            );
        })

        .test("deserialize strings with various escapes", [] {
            std::istringstream istream(
                "The End;The\\; End;The\\\\ End;The\\\\\\\\ End;The\\\\\\; End;"
            );
            PlaintextDeserializer deserializer_impl(istream);
            Deserializer& deserializer = deserializer_impl;

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

        .test("deserialize past beyond end error", [] {
            std::istringstream istream("2;");
            PlaintextDeserializer deserializer_impl(istream);
            Deserializer& deserializer = deserializer_impl;

            int64_t int_field;
            uint64_t actual = 0;

            deserializer >> int_field;

            bool throwed = false;
            try {
                 deserializer >> actual;
            } catch (DeserializationError const &exception) {
                throwed = true;
            }

            TEST_ASSERT(
                std::string("int field, found: ") + std::to_string(int_field),
                int_field == 2
            );
            TEST_ASSERT(
                std::string("should throw, but found ") + std::to_string(actual),
                throwed
            );
        })

        .test("deserialize bad signed int chars", [] {
            std::istringstream istream("a;");
            PlaintextDeserializer deserializer_impl(istream);
            Deserializer& deserializer = deserializer_impl;

            int64_t actual;

            bool throwed = false;
            try {
                 deserializer >> actual;
            } catch (DeserializationError const &exception) {
                throwed = true;
            }
            TEST_ASSERT(
                std::string("should throw, but found ")
                    + std::to_string(actual),
                throwed
            );
        })

        .test("deserialize signed int out of range", [] {
            std::istringstream istream("-1230;");
            PlaintextDeserializer deserializer_impl(istream);
            Deserializer& deserializer = deserializer_impl;

            int8_t actual;

            bool throwed = false;
            try {
                 deserializer >> actual;
            } catch (DeserializationError const &exception) {
                throwed = true;
            }
            TEST_ASSERT(
                std::string("should throw, but found ")
                    + std::to_string(actual),
                throwed
            );
        })

        .test("deserialize bad signal for unsigned int", [] {
            std::istringstream istream("-1230;");
            PlaintextDeserializer deserializer_impl(istream);
            Deserializer& deserializer = deserializer_impl;

            uint16_t actual;

            bool throwed = false;
            try {
                 deserializer >> actual;
            } catch (DeserializationError const &exception) {
                throwed = true;
            }
            TEST_ASSERT(
                std::string("should throw, but found ")
                    + std::to_string(actual),
                throwed
            );
        })


        .test("deserialize unsigned int out of range", [] {
            std::istringstream istream("1230;");
            PlaintextDeserializer deserializer_impl(istream);
            Deserializer& deserializer = deserializer_impl;

            uint8_t actual;

            bool throwed = false;
            try {
                 deserializer >> actual;
            } catch (DeserializationError const &exception) {
                throwed = true;
            }
            TEST_ASSERT(
                std::string("should throw, but found ")
                    + std::to_string(actual),
                throwed
            );
        })

        .test("deserialize empty int chars", [] {
            std::istringstream istream(";");
            PlaintextDeserializer deserializer_impl(istream);
            Deserializer& deserializer = deserializer_impl;

            int64_t actual;

            bool throwed = false;
            try {
                 deserializer >> actual;
            } catch (DeserializationError const &exception) {
                throwed = true;
            }
            TEST_ASSERT(
                std::string("should throw, but found ")
                    + std::to_string(actual),
                throwed
            );
        })
    ;
}

static TestSuite socket_test_suite()
{
    return TestSuite()
        .test("basic send and receive", [] {
            Socket client(500);
            Socket server(500, 8082);
            Enveloped enveloped;
            enveloped.remote = Address(make_ipv4({ 127, 0, 0, 1 }), 8082);
            enveloped.message;
            enveloped.message.header = MessageHeader::gen_request();
            enveloped.message.body = std::shared_ptr<MessageConnectReq>(
                new MessageConnectReq("bruno")
            );
            client.send(message);

            Enveloped received = server.receive();

            TEST_ASSERT(
                std::string("basic recevied ipv4 address is wrong, address: ")
                    + received.remote.to_string(),
                received.remote.ipv4 == make_ipv4({ 127, 0, 0, 1 })
            );

            TEST_ASSERT(
                std::string("basic recevied port is wrong, address: ")
                    + received.remote.to_string(),
                received.remote.port != 8082
            );

            TEST_ASSERT(
                "found message tag: "
                    + received.message.body->tag().to_string(),
                received.message.body->tag() == MessageTag(MSG_REQ, MSG_CONNECT)
            );

            MessageConnectReq const& casted_body =
                dynamic_cast<MessageConnectReq const&>(*received.message.body);

            TEST_ASSERT(
                "message username should be \"bruno\", found: "
                    + casted_body.username,
                casted_body.username == "bruno"
            );
        })

        .test("full message workflow", [] {
            Socket client(500);
            Socket server(500, 8082);

            Enveloped request;
            request.remote = Address(make_ipv4({ 127, 0, 0, 1 }), 8082);
            request.message.header = MessageHeader::gen_request();
            request.message.body = std::shared_ptr<MessageConnectReq>(
                new MessageConnectReq("bruno")
            );
            client.send(request);

            Enveloped received_req = server.receive();

            TEST_ASSERT(
                "found " + received_req.message.body->tag().to_string(),
                received_req.message.body->tag()
                    == MessageTag(MSG_REQ, MSG_CONNECT)
            );

            MessageConnectReq const& casted_req_body =
                dynamic_cast<MessageConnectReq const&>(
                    *received_req.message.body
                );

            TEST_ASSERT(
                "message username should be \"bruno\", found: "
                    + casted_req_body.username,
                casted_req_body.username == "bruno"
            );

            Enveloped response;
            response.remote = received_req.remote;
            response.enveloped.header =
                MessageHeader::gen_response(received_req.header.seqn);
            response.enveloped.body = std::shared_ptr<MessageConnectResp>(
                new MessageConnectResp(MSG_OK)
            );
            server.send(response);

            Enveloped received_resp = client.receive();

            TEST_ASSERT(
                "found " + received_resp.body->tag().to_string(),
                received_resp.message.body->tag()
                    == MessageTag(MSG_RESP, MSG_CONNECT)
            );

            MessageConnectResp const& casted_resp_body =
                dynamic_cast<MessageConnectResp const&>(
                    *received_resp.message.body
                );

            TEST_ASSERT(
                "message status should be MSG_OK, found: "
                    + casted_resp_body.status,
                casted_resp_body.status == MSG_OK
            );
        })
;
}

static TestSuite channel_test_suite()
{
    return TestSuite()
        .test("single producer, single consumer, sender ends first", [] {
            Channel<uint64_t> channel;
            Channel<uint64_t>::Sender sender = std::move(channel.sender);
            Channel<uint64_t>::Receiver receiver = std::move(channel.receiver);
            constexpr uint64_t total_messages = 12345;
            std::thread sender_thread ([sender = std::move(sender)] () mutable {
                for (uint64_t i = 0; i < total_messages; i++) {
                    sender.send(i);
                }
            });

            bool disconnected = false;

            uint64_t message_count = 0;

            while (!disconnected) {
                try {
                    uint64_t message = receiver.receive();
                    TEST_ASSERT(
                        std::string("message should be ")
                            + std::to_string(message_count)
                            + ", found "
                            + std::to_string(message),
                        message == message_count
                    );
                    message_count++;
                } catch (SendersDisconnected const& exc) {
                    disconnected = true;
                }
            }

            sender_thread.join();

            TEST_ASSERT(
                std::string("found actual message count to be ")
                    + std::to_string(message_count),
                message_count == total_messages
            );
        })

        .test("single producer, single consumer, receiver ends first", [] {
            Channel<uint64_t> channel;
            Channel<uint64_t>::Sender sender = std::move(channel.sender);
            {
                Channel<uint64_t>::Receiver receiver =
                    std::move(channel.receiver);
            }

            bool disconnected = false;
            try {
                sender.send(3);
            } catch (ReceiversDisconnected const& exception) {
                disconnected = true;
            }

            TEST_ASSERT(
                std::string("should have disconnected, did it? ")
                    + (disconnected ? "true" : "false"),
                disconnected
            );
        })

        .test("multi producer, multi consumer", [] {
            std::set<uint64_t> received_messages;
            std::mutex message_mutex;
            Channel<uint64_t> channel;
            Channel<uint64_t>::Sender sender = std::move(channel.sender);
            Channel<uint64_t>::Receiver receiver = std::move(channel.receiver);
            constexpr uint64_t total_receivers = 4;
            constexpr uint64_t total_senders = 4;
            constexpr uint64_t total_messages = 48000;
            constexpr uint64_t messages_per_sender =
                total_messages / total_senders;

            std::vector<std::thread> sender_threads;

            for (uint64_t i = 0; i < total_senders; i++) {
                sender_threads.push_back(
                    std::thread([i, sender] () mutable {
                        for (uint64_t j = 0; j < messages_per_sender; j++) {
                            sender.send(j + i * messages_per_sender);
                        }
                    })
                );
            }

            std::vector<std::thread> receiver_threads;

            for (uint64_t i = 0; i < total_receivers; i++) {
                receiver_threads.push_back(
                    std::thread([
                        receiver,
                        &message_mutex,
                        &received_messages
                    ] () mutable {
                        bool disconnected = false;
                        std::set<uint64_t> messages;
                        while (!disconnected) {
                            try {
                                uint64_t message = receiver.receive();
                                messages.insert(message);
                            } catch (SendersDisconnected const& exc) {
                                disconnected = true;
                            }
                        }

                        std::unique_lock lock(message_mutex);
                        received_messages.merge(messages);
                    })
                );
            }

            {
                Channel<uint64_t>::Sender drop_sender = std::move(sender);
                Channel<uint64_t>::Receiver drop_receiver = std::move(receiver);
            }

            for (auto& sender_thread : sender_threads) {
                sender_thread.join();
            }
            for (auto& receiver_thread : receiver_threads) {
                receiver_thread.join();
            }

            TEST_ASSERT(
                std::string("total number of received messages is ")
                    + std::to_string(received_messages.size()),
                total_messages == received_messages.size()
            );

            uint64_t max_message = *received_messages.crbegin();
            TEST_ASSERT(
                std::string("maximum message is ")
                    + std::to_string(max_message),
                max_message = total_messages - 1
            );
        })
;
}
