#include <set>
#include <thread>
#include <atomic>
#include "shared.h"
#include "../shared/address.h"
#include "../shared/message.h"
#include "../shared/socket.h"
#include "../shared/channel.h"
#include "../shared/tracker.h"
#include "../shared/username.h"
#include "../shared/notif_message.h"
#include "../shared/string_ext.h"

static TestSuite parse_udp_port_test_suite();
static TestSuite parse_ipv4_test_suite();
static TestSuite plaintext_ser_test_suite();
static TestSuite plaintext_de_test_suite();
static TestSuite socket_test_suite();
static TestSuite channel_test_suite();
static TestSuite reliable_socket_test_suite();
static TestSuite thread_tracker_test_suite();
static TestSuite username_test_suite();
static TestSuite notif_message_test_suite();
static TestSuite string_ext_test_suite();
static TestSuite seqn_set_test_suite();

TestSuite shared_test_suite()
{
    return TestSuite()
        .append(seqn_set_test_suite())
        .append(parse_udp_port_test_suite())
        .append(parse_ipv4_test_suite())
        .append(plaintext_ser_test_suite())
        .append(plaintext_de_test_suite())
        .append(socket_test_suite())
        .append(channel_test_suite())
        .append(reliable_socket_test_suite())
        .append(thread_tracker_test_suite())
        .append(username_test_suite())
        .append(notif_message_test_suite())
        .append(string_ext_test_suite())
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

static TestSuite parse_ipv4_test_suite()
{
    return TestSuite()
        .test("parse_ipv4 valid local", [] {
            uint32_t actual = parse_ipv4("192.168.15.132");
            
            TEST_ASSERT(
                std::string("found ") + ipv4_to_string(actual),
                ipv4_to_string(actual) == "192.168.15.132"
            );
        })

        .test("parse_ipv4 valid spaced", [] {
            uint32_t actual = parse_ipv4("  192 .168.15.    132 ");
            
            TEST_ASSERT(
                std::string("found ") + ipv4_to_string(actual),
                ipv4_to_string(actual) == "192.168.15.132"
            );
        })

        .test("parse_ipv4 valid loopback", [] {
            uint32_t actual = parse_ipv4("127.0.0.1");
            
            TEST_ASSERT(
                std::string("found ") + ipv4_to_string(actual),
                ipv4_to_string(actual) == "127.0.0.1"
            );
        })

        .test("parse_ipv4 missing byte", [] {
            bool thrown = false;
            uint32_t actual = 0;
            try {
                actual = parse_ipv4("127.0.0.");
            } catch (InvalidIpv4 const& exc) {
                thrown = true;
            }
            
            TEST_ASSERT(
                std::string("should throw, but found ") + ipv4_to_string(actual),
                thrown
            );
        })

        .test("parse_ipv4 bad char", [] {
            bool thrown = false;
            uint32_t actual = 0;
            try {
                actual = parse_ipv4("127&0.0.1");
            } catch (InvalidIpv4 const& exc) {
                thrown = true;
            }
            
            TEST_ASSERT(
                std::string("should throw, but found ") + ipv4_to_string(actual),
                thrown
            );
        })

        .test("parse_ipv4 bad trailing chars", [] {
            bool thrown = false;
            uint32_t actual = 0;
            try {
                actual = parse_ipv4("127&0.0.1 .");
            } catch (InvalidIpv4 const& exc) {
                thrown = true;
            }
            
            TEST_ASSERT(
                std::string("should throw, but found ") + ipv4_to_string(actual),
                thrown
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
            Socket server(Address(make_ipv4({ 127, 0, 0, 1 }), 8082), 500);
            Enveloped enveloped;
            enveloped.remote = Address(make_ipv4({ 127, 0, 0, 1 }), 8082);
            enveloped.message;
            enveloped.message.header.fill_req();
            enveloped.message.body = std::shared_ptr<MessageBody>(
                new MessageClientConnReq(Username("@bruno"))
            );
            client.send(enveloped);

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
                received.message.body->tag() == MessageTag(MSG_REQ, MSG_CLIENT_CONN)
            );

            MessageClientConnReq const& casted_body =
                received.message.body->cast<MessageClientConnReq>();

            TEST_ASSERT(
                std::string("message username should be \"bruno\", found: ")
                    + casted_body.username.content(),
                casted_body.username == Username("@bruno")
            );
        })

        .test("full message workflow", [] {
            Socket client(500);
            Socket server(Address(make_ipv4({ 127, 0, 0, 1 }), 8082), 500);

            Enveloped request;
            request.remote = Address(make_ipv4({ 127, 0, 0, 1 }), 8082);
            request.message.header.fill_req();
            request.message.body = std::shared_ptr<MessageBody>(
                new MessageClientConnReq(Username("@bruno"))
            );
            client.send(request);

            Enveloped received_req = server.receive();

            TEST_ASSERT(
                "found " + received_req.message.body->tag().to_string(),
                received_req.message.body->tag()
                    == MessageTag(MSG_REQ, MSG_CLIENT_CONN)
            );

            MessageClientConnReq const& casted_req_body =
                received_req.message.body->cast<MessageClientConnReq>();

            TEST_ASSERT(
                "message username should be \"bruno\", found: "
                    + casted_req_body.username.content(),
                casted_req_body.username == Username("@bruno")
            );

            Enveloped response;
            response.remote = received_req.remote;
            response.message.header.fill_resp(received_req.message.header.seqn);
            response.message.body = std::shared_ptr<MessageClientConnResp>(
                new MessageClientConnResp
            );
            server.send(response);

            Enveloped received_resp = client.receive();

            TEST_ASSERT(
                "found " + received_resp.message.body->tag().to_string(),
                received_resp.message.body->tag()
                    == MessageTag(MSG_RESP, MSG_CLIENT_CONN)
            );

            MessageClientConnResp const& casted_resp_body =
                received_resp.message.body->cast<MessageClientConnResp>();
        })
;
}

static TestSuite channel_test_suite()
{
    return TestSuite()
        .test("channel with no message", [] {
            Channel<uint64_t> channel;
            Channel<uint64_t>::Sender sender = std::move(channel.sender);
            Channel<uint64_t>::Receiver receiver = std::move(channel.receiver);
            constexpr uint64_t total_messages = 0;
            std::thread sender_thread ([sender = std::move(sender)] () mutable {
                sender.disconnect();
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

static TestSuite reliable_socket_test_suite()
{
    return TestSuite()
        .test("one client, one server, single-threaded", [] () {
            Socket client_udp(500);
            ReliableSocket client(std::move(client_udp));

            Socket server_udp(Address(make_ipv4({ 127, 0, 0, 1 }), 8082), 500);
            ReliableSocket server(std::move(server_udp));

            Enveloped conn_req;
            conn_req.remote = Address(make_ipv4({ 127, 0, 0, 1 }), 8082);
            conn_req.message.body = std::shared_ptr<MessageBody>(
                new MessageClientConnReq(Username("@bruno"))
            );
            ReliableSocket::SentReq sent_conn_req = client.send_req(conn_req);

            ReliableSocket::ReceivedReq recvd_conn_req = server.receive_req();
            TEST_ASSERT(
                "found " + recvd_conn_req.req_enveloped()
                    .message.body->tag().to_string(),
                recvd_conn_req.req_enveloped().message.body->tag()
                    == MessageTag(MSG_REQ, MSG_CLIENT_CONN)
            );

            std::move(recvd_conn_req).send_resp(std::shared_ptr<MessageBody>(
                new MessageClientConnResp
            ));

            Enveloped recvd_conn_resp = std::move(sent_conn_req).receive_resp();
            TEST_ASSERT(
                "found " + recvd_conn_resp.message.body->tag().to_string(),
                recvd_conn_resp.message.body->tag()
                    == MessageTag(MSG_RESP, MSG_CLIENT_CONN)
            );

            Enveloped disconn_req;
            disconn_req.remote = Address(make_ipv4({ 127, 0, 0, 1 }), 8082);
            disconn_req.message.body = std::shared_ptr<MessageBody>(
                new MessageDisconnectReq
            );
            ReliableSocket::SentReq sent_disconn_req =
                client.send_req(disconn_req);

            ReliableSocket::ReceivedReq recvd_disconn_req =
                server.receive_req();
            TEST_ASSERT(
                "found " + recvd_disconn_req.req_enveloped()
                    .message.body->tag().to_string(),
                recvd_disconn_req.req_enveloped().message.body->tag()
                    == MessageTag(MSG_REQ, MSG_DISCONNECT)
            );

            std::move(recvd_disconn_req).send_resp(std::shared_ptr<MessageBody>(
                new MessageDisconnectResp
            ));

            Enveloped recvd_disconn_resp =
                std::move(sent_disconn_req).receive_resp();
            TEST_ASSERT(
                "found " + recvd_disconn_resp.message.body->tag().to_string(),
                recvd_disconn_resp.message.body->tag()
                    == MessageTag(MSG_RESP, MSG_DISCONNECT)
            );
        })

        .test("one client, one server, multi-threaded", [] () {
            Socket client_udp(500);
            ReliableSocket client(std::move(client_udp));

            Socket server_udp(Address(make_ipv4({ 127, 0, 0, 1 }), 8082), 500);
            ReliableSocket server(std::move(server_udp));

            std::thread server_thread([server = std::move(server)] () mutable {
                ReliableSocket::ReceivedReq recvd_conn_req =
                    server.receive_req();
                TEST_ASSERT(
                    "found " + recvd_conn_req.req_enveloped()
                        .message.body->tag().to_string(),
                    recvd_conn_req.req_enveloped().message.body->tag()
                        == MessageTag(MSG_REQ, MSG_CLIENT_CONN)
                );

                std::move(recvd_conn_req).send_resp(
                    std::shared_ptr<MessageBody>(new MessageClientConnResp)
                );

                ReliableSocket::ReceivedReq recvd_disconn_req =
                    server.receive_req();
                TEST_ASSERT(
                    "found " + recvd_disconn_req.req_enveloped()
                        .message.body->tag().to_string(),
                    recvd_disconn_req.req_enveloped().message.body->tag()
                        == MessageTag(MSG_REQ, MSG_DISCONNECT)
                );

                std::move(recvd_disconn_req).send_resp(
                    std::shared_ptr<MessageBody>(new MessageDisconnectResp)
                );
            });

            Enveloped conn_req;
            conn_req.remote = Address(make_ipv4({ 127, 0, 0, 1 }), 8082);
            conn_req.message.body = std::shared_ptr<MessageBody>(
                new MessageClientConnReq(Username("@bruno"))
            );
            ReliableSocket::SentReq sent_conn_req = client.send_req(conn_req);

            Enveloped recvd_conn_resp = std::move(sent_conn_req).receive_resp();
            TEST_ASSERT(
                "found " + recvd_conn_resp.message.body->tag().to_string(),
                recvd_conn_resp.message.body->tag()
                    == MessageTag(MSG_RESP, MSG_CLIENT_CONN)
            );

            Enveloped disconn_req;
            disconn_req.remote = Address(make_ipv4({ 127, 0, 0, 1 }), 8082);
            disconn_req.message.body = std::shared_ptr<MessageBody>(
                new MessageDisconnectReq
            );
            ReliableSocket::SentReq sent_disconn_req =
                client.send_req(disconn_req);

            Enveloped recvd_disconn_resp =
                std::move(sent_disconn_req).receive_resp();
            TEST_ASSERT(
                "found " + recvd_disconn_resp.message.body->tag().to_string(),
                recvd_disconn_resp.message.body->tag()
                    == MessageTag(MSG_RESP, MSG_DISCONNECT)
            );

            server_thread.join();
        })

        .test("multi client, multi-threaded", [] {
            Socket server_udp(Address(make_ipv4({ 127, 0, 0, 1 }), 8082), 500);
            ReliableSocket server(std::move(server_udp));

            constexpr size_t thread_count = 4;

            std::vector<std::thread> client_threads;

            for (size_t i = 0; i < thread_count; i++) {
                client_threads.push_back(std::move(std::thread([] () mutable {
                    Socket client_udp(500);
                    ReliableSocket client(std::move(client_udp));
                    Enveloped conn_req;
                    conn_req.remote =
                        Address(make_ipv4({ 127, 0, 0, 1 }), 8082);
                    conn_req.message.body = std::shared_ptr<MessageBody>(
                        new MessageClientConnReq(Username("@bruno"))
                    );
                    ReliableSocket::SentReq sent_conn_req =
                        client.send_req(conn_req);

                    Enveloped recvd_conn_resp =
                        std::move(sent_conn_req).receive_resp();
                    TEST_ASSERT(
                        "found "
                            + recvd_conn_resp.message.body->tag().to_string(),
                        recvd_conn_resp.message.body->tag()
                            == MessageTag(MSG_RESP, MSG_CLIENT_CONN)
                    );

                    Enveloped disconn_req;
                    disconn_req.remote =
                        Address(make_ipv4({ 127, 0, 0, 1 }), 8082);
                    disconn_req.message.body = std::shared_ptr<MessageBody>(
                        new MessageDisconnectReq
                    );
                    ReliableSocket::SentReq sent_disconn_req =
                        client.send_req(disconn_req);

                    Enveloped recvd_disconn_resp =
                        std::move(sent_disconn_req).receive_resp();
                    TEST_ASSERT(
                        "found "
                            + recvd_disconn_resp.message.body->tag()
                            .to_string(),
                        recvd_disconn_resp.message.body->tag()
                            == MessageTag(MSG_RESP, MSG_DISCONNECT)
                    );
                })));
            }

            size_t connected = 0;
            size_t disconnected = 0;

            while (disconnected < thread_count) {
                ReliableSocket::ReceivedReq received = server.receive_req();
                switch (received.req_enveloped().message.body->tag().type) {
                    case MSG_CLIENT_CONN:
                        std::move(received)
                            .send_resp(std::shared_ptr<MessageBody>(
                                new MessageClientConnResp
                            ));
                        connected++;
                        break;
                    case MSG_DISCONNECT:
                        std::move(received)
                            .send_resp(std::shared_ptr<MessageBody>(
                                new MessageDisconnectResp
                            ));
                        disconnected++;
                        break;
                    default:
                        TEST_ASSERT(
                            std::string("found: ")
                                + received.req_enveloped().message.body->tag()
                                .to_string(),
                            false
                        );
                        break;
                }
            }

            for (auto& thread : client_threads) {
                thread.join();
            }

            TEST_ASSERT(
                std::string("connected: ") + std::to_string(connected),
                thread_count == connected
            );
            TEST_ASSERT(
                std::string("disconnected: ") + std::to_string(disconnected),
                thread_count == disconnected
            );
        })
    ;
}

static TestSuite thread_tracker_test_suite()
{
    return TestSuite()
        .test("simple tracking", [] {
            std::shared_ptr<std::atomic<uint32_t>> counter =
                std::shared_ptr<std::atomic<uint32_t>>(
                    new std::atomic<uint32_t>(0)
                );

            constexpr uint32_t thread_count = 500;

            {
                using namespace std::chrono_literals;

                ThreadTracker thread_tracker;

                for (uint32_t i = 0; i < thread_count; i++) {
                    thread_tracker.spawn([counter] {
                        std::this_thread::sleep_for(10us);
                        counter->fetch_add(1);
                    });
                }
            }

            uint32_t actual_count = counter->load();

            TEST_ASSERT(
                std::string("found count: ") + std::to_string(actual_count),
                thread_count == actual_count
            );
        })
    ;
}

static TestSuite username_test_suite()
{
    return TestSuite()
        .test("min username is accepted", [] {
            Username("@abcd");
        })
        .test("max username is accepted", [] {
            Username("@abcdef0123456789abcd");
        })
        .test("username below min is rejected", [] {
            bool thrown = false;
            try {
                Username("@abc");
            } catch (InvalidUsername const& exc) {
                thrown = true;
            }
            TEST_ASSERT("should have thrown", thrown);
        })
        .test("username above max is rejected", [] {
            bool thrown = false;
            try {
                Username("@abcdef0123456789abcde");
            } catch (InvalidUsername const& exc) {
                thrown = true;
            }
            TEST_ASSERT("should have thrown", thrown);
        })
        .test("invalid username byte is rejected", [] {
            bool thrown = false;
            try {
                Username("@bruno!");
            } catch (InvalidUsername const& exc) {
                thrown = true;
            }
            TEST_ASSERT("should have thrown", thrown);
        })
    ;
}

static TestSuite notif_message_test_suite()
{
    return TestSuite()
        .test("min notification is accepted", [] {
            NotifMessage("a");
        })
        .test("max notification is accepted", [] {
            NotifMessage(
                std::string("0123456789abcdef")
                    + "0123456789abcdef"
                    + "0123456789abcdef"
                    + "0123456789abcdef"

                    + "0123456789abcdef"
                    + "0123456789abcdef"
                    + "0123456789abcdef"
                    + "0123456789abcdef"
            );
        })
        .test("notification below min is rejected", [] {
            bool thrown = false;
            try {
                NotifMessage("");
            } catch (InvalidNotifMessage const& exc) {
                thrown = true;
            }
            TEST_ASSERT("should have thrown", thrown);
        })
        .test("notification above max is rejected", [] {
            bool thrown = false;
            try {
                NotifMessage(
                    std::string("0123456789abcdef")
                        + "0123456789abcdef"
                        + "0123456789abcdef"
                        + "0123456789abcdef"

                        + "0123456789abcdef"
                        + "0123456789abcdef"
                        + "0123456789abcdef"
                        + "0123456789abcdef0"
                );
            } catch (InvalidNotifMessage const& exc) {
                thrown = true;
            }
            TEST_ASSERT("should have thrown", thrown);
        })
    ;
}

static TestSuite string_ext_test_suite()
{
    return TestSuite()
        .test("trim nothing", [] () {
            std::string target = "a bc";
            std::string trimmed = trim_spaces(target);

            TEST_ASSERT(
                "found: " + trimmed,
                trimmed == "a bc"
            );
        })

        .test("trim only start", [] () {
            std::string target = "  a bc";
            std::string trimmed = trim_spaces(target);

            TEST_ASSERT(
                "found: " + trimmed,
                trimmed == "a bc"
            );
        })

        .test("trim only end", [] () {
            std::string target = "a bc   ";
            std::string trimmed = trim_spaces(target);

            TEST_ASSERT(
                "found: " + trimmed,
                trimmed == "a bc"
            );
        })

        .test("trim start and end", [] () {
            std::string target = "  a bc   ";
            std::string trimmed = trim_spaces(target);

            TEST_ASSERT(
                "found: " + trimmed,
                trimmed == "a bc"
            );
        })

        .test("trim empty", [] () {
            std::string target = "  ";
            std::string trimmed = trim_spaces(target);

            TEST_ASSERT(
                "found: " + trimmed,
                trimmed == ""
            );
        })

        .test("starts with is true, superstring", [] () {
            std::string haystack = "Hey, John!";
            std::string needle = "HeY";

            TEST_ASSERT(
                "should start with",
                string_starts_with_ignore_case(haystack, needle)
            );
        })

        .test("starts with is true, equals", [] () {
            std::string haystack = "Hey";
            std::string needle = "hey";

            TEST_ASSERT(
                "should start with",
                string_starts_with_ignore_case(haystack, needle)
            );
        })

        .test("starts with is false, superstring", [] () {
            std::string haystack = "Hey, John!";
            std::string needle = "Hea";

            TEST_ASSERT(
                "should not start with",
                !string_starts_with_ignore_case(haystack, needle)
            );
        })

        .test("starts with is false, equals", [] () {
            std::string haystack = "Hey";
            std::string needle = "jey";

            TEST_ASSERT(
                "should not start with",
                !string_starts_with_ignore_case(haystack, needle)
            );
        })

        .test("starts with is false, substring", [] () {
            std::string haystack = "He";
            std::string needle = "Hey";

            TEST_ASSERT(
                "should not start with",
                !string_starts_with_ignore_case(haystack, needle)
            );
        })
    ;
}

static TestSuite seqn_set_test_suite()
{
    return TestSuite()
        .test("initial add zero twice", [] {
            SeqnSet set;
            TEST_ASSERT(
                "should add",
                set.add(0)
            );
            TEST_ASSERT(
                "should not add",
                !set.add(0)
            );
            TEST_ASSERT(
                "should contain 0",
                set.contains(0)
            );
        })
        .test("initial add two twice", [] {
            SeqnSet set;
            TEST_ASSERT(
                "should add",
                set.add(2)
            );
            TEST_ASSERT(
                "should not add",
                !set.add(2)
            );
        })
        .test("mix two and zero", [] {
            SeqnSet set;
            TEST_ASSERT(
                "should add",
                set.add(0)
            );
            TEST_ASSERT(
                "should add",
                set.add(2)
            );
            TEST_ASSERT(
                "should not add",
                !set.add(0)
            );
            TEST_ASSERT(
                "should not add",
                !set.add(2)
            );
        })

        .test("mix zero and two", [] {
            SeqnSet set;
            TEST_ASSERT(
                "should add",
                set.add(2)
            );
            TEST_ASSERT(
                "should add",
                set.add(0)
            );
            TEST_ASSERT(
                "should not add",
                !set.add(2)
            );
            TEST_ASSERT(
                "should not add",
                !set.add(0)
            );
            TEST_ASSERT(
                "should contain 0",
                set.contains(0)
            );
            TEST_ASSERT(
                "should contain 2",
                set.contains(2)
            );
        })

        .test("merge zero and one and two", [] {
            SeqnSet set;
            TEST_ASSERT(
                "should add",
                set.add(2)
            );
            TEST_ASSERT(
                "should add",
                set.add(0)
            );
            TEST_ASSERT(
                "should add",
                set.add(1)
            );
            TEST_ASSERT(
                "should not add",
                !set.add(1)
            );
            TEST_ASSERT(
                "should not add",
                !set.add(2)
            );
            TEST_ASSERT(
                "should not add",
                !set.add(0)
            );
        })

        .test("add many", [] {
            SeqnSet set;
            TEST_ASSERT(
                "should add",
                set.add(2)
            );
            TEST_ASSERT(
                "should add",
                set.add(6)
            );
            TEST_ASSERT(
                "should add",
                set.add(4)
            );
            TEST_ASSERT(
                "should add",
                set.add(3)
            );
            TEST_ASSERT(
                "should add",
                set.add(7)
            );
            TEST_ASSERT(
                "should add",
                set.add(5)
            );
            TEST_ASSERT(
                "should not add",
                !set.add(2)
            );
            TEST_ASSERT(
                "should not add",
                !set.add(3)
            );
            TEST_ASSERT(
                "should not add",
                !set.add(4)
            );
            TEST_ASSERT(
                "should not add",
                !set.add(5)
            );
            TEST_ASSERT(
                "should not add",
                !set.add(6)
            );
            TEST_ASSERT(
                "should not add",
                !set.add(7)
            );
            TEST_ASSERT(
                "should add",
                set.add(9)
            );
            TEST_ASSERT(
                "should not add",
                !set.add(9)
            );
            TEST_ASSERT(
                "should add",
                set.add(8)
            );
            TEST_ASSERT(
                "should not add",
                !set.add(8)
            );
            TEST_ASSERT(
                "should not add",
                !set.add(9)
            );
            TEST_ASSERT(
                "should not add",
                !set.add(3)
            );
            TEST_ASSERT(
                "should contain 2",
                set.contains(2)
            );
            TEST_ASSERT(
                "should contain 9",
                set.contains(9)
            );
            TEST_ASSERT(
                "should contain 7",
                set.contains(7)
            );
            TEST_ASSERT(
                "should contain 4",
                set.contains(4)
            );
        })

        .test("remove seqn splitting", [] {
            SeqnSet set;
            TEST_ASSERT(
                "should add 2",
                set.add(2)
            );
            TEST_ASSERT(
                "should add 3",
                set.add(3)
            );
            TEST_ASSERT(
                "should add 4",
                set.add(4)
            );
            TEST_ASSERT(
                "should add 5",
                set.add(5)
            );
            TEST_ASSERT(
                "should remove 3",
                set.remove(3)
            );
            TEST_ASSERT(
                "should contain 2",
                set.contains(2)
            );
            TEST_ASSERT(
                "should not contain 3",
                !set.contains(3)
            );
            TEST_ASSERT(
                "should contain 5",
                set.contains(5)
            );
            TEST_ASSERT(
                "should contain 4",
                set.contains(4)
            );
        })

        .test("not remove seqn", [] {
            SeqnSet set;
            TEST_ASSERT(
                "should add 2",
                set.add(2)
            );
            TEST_ASSERT(
                "should add 3",
                set.add(3)
            );
            TEST_ASSERT(
                "should add 4",
                set.add(4)
            );
            TEST_ASSERT(
                "should add 5",
                set.add(5)
            );
            TEST_ASSERT(
                "should not remove 0",
                !set.remove(0)
            );
            TEST_ASSERT(
                "should remove 3",
                set.remove(3)
            );
            TEST_ASSERT(
                "should not remove 3",
                !set.remove(3)
            );
            TEST_ASSERT(
                "should contain 2",
                set.contains(2)
            );
            TEST_ASSERT(
                "should not contain 3",
                !set.contains(3)
            );
            TEST_ASSERT(
                "should contain 5",
                set.contains(5)
            );
            TEST_ASSERT(
                "should contain 4",
                set.contains(4)
            );
        })

        .test("remove seqn below", [] {
            SeqnSet set;
            TEST_ASSERT(
                "should add 0",
                set.add(0)
            );
            TEST_ASSERT(
                "should add 2",
                set.add(2)
            );
            TEST_ASSERT(
                "should add 3",
                set.add(3)
            );
            TEST_ASSERT(
                "should add 4",
                set.add(4)
            );
            TEST_ASSERT(
                "should add 5",
                set.add(5)
            );
            TEST_ASSERT(
                "should add 7",
                set.add(7)
            );
            TEST_ASSERT(
                "should remove 2",
                set.remove(2)
            );
            TEST_ASSERT(
                "should not contain 2",
                !set.contains(2)
            );
            TEST_ASSERT(
                "should contain 4",
                set.contains(4)
            );
            TEST_ASSERT(
                "should contain 5",
                set.contains(5)
            );
            TEST_ASSERT(
                "should contain 3",
                set.contains(3)
            );
            TEST_ASSERT(
                "should contain 0",
                set.contains(0)
            );
            TEST_ASSERT(
                "should contain 7",
                set.contains(7)
            );
        })
        
        .test("remove seqn above", [] {
            SeqnSet set;
            TEST_ASSERT(
                "should add 0",
                set.add(0)
            );
            TEST_ASSERT(
                "should add 2",
                set.add(2)
            );
            TEST_ASSERT(
                "should add 3",
                set.add(3)
            );
            TEST_ASSERT(
                "should add 4",
                set.add(4)
            );
            TEST_ASSERT(
                "should add 5",
                set.add(5)
            );
            TEST_ASSERT(
                "should add 7",
                set.add(7)
            );
            TEST_ASSERT(
                "should remove 5",
                set.remove(5)
            );
            TEST_ASSERT(
                "should not contain 5",
                !set.contains(5)
            );
            TEST_ASSERT(
                "should contain 4",
                set.contains(4)
            );
            TEST_ASSERT(
                "should contain 2",
                set.contains(2)
            );
            TEST_ASSERT(
                "should contain 3",
                set.contains(3)
            );
            TEST_ASSERT(
                "should contain 0",
                set.contains(0)
            );
            TEST_ASSERT(
                "should contain 7",
                set.contains(7)
            );
        })

        .test("remove seqn isolated", [] {
            SeqnSet set;
            TEST_ASSERT(
                "should add 0",
                set.add(0)
            );
            TEST_ASSERT(
                "should add 3",
                set.add(3)
            );
            TEST_ASSERT(
                "should add 7",
                set.add(7)
            );
            TEST_ASSERT(
                "should remove 3",
                set.remove(3)
            );
            TEST_ASSERT(
                "should not contain 3",
                !set.contains(3)
            );
            TEST_ASSERT(
                "should contain 0",
                set.contains(0)
            );
            TEST_ASSERT(
                "should contain 7",
                set.contains(7)
            );
        })

        .test("missing below", [] {
            SeqnSet set;
            TEST_ASSERT(
                "should add",
                set.add(2)
            );
            TEST_ASSERT(
                "should add",
                set.add(6)
            );
            TEST_ASSERT(
                "should add",
                set.add(4)
            );
            TEST_ASSERT(
                "should add",
                set.add(3)
            );
            TEST_ASSERT(
                "should add",
                set.add(9)
            );
            TEST_ASSERT(
                "should add",
                set.add(5)
            );

            std::set<uint64_t> seqns = set.missing_below();

            std::string actual;

            for (auto seqn : seqns) {
                actual += std::to_string(seqn) + " ";
            }

            std::set<uint64_t> expected { 0, 1, 7, 8 };

            TEST_ASSERT(
                "expected seqns, found: " + actual,
                seqns == expected
            );
        })
    ;
}
