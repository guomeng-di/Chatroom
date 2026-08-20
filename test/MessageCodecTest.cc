#include <gtest/gtest.h>

#include "../protocol/MessageCodec/MessageCodec.h"

#include <nlohmann/json.hpp>

using namespace std;
using json = nlohmann::json;


// 测试普通消息编码
TEST(MessageCodecTest, Encode)
{
    json js;

    js["msgid"] = 1;
    js["username"] = "tom";
    js["password"] = "1234";

    string message = js.dump();

    string encoded = MessageCodec::encode(message);

    // 编码后：
    // | totalLen(4) | msgid(4) | json |

    EXPECT_GT(encoded.size(), 8);

    // 注意：
    // getMsgId()要求传入的是：
    // | msgid(4) | json |
    //
    // 所以跳过前4字节totalLen
    string messageWithMsgId =
        encoded.substr(4);

    EXPECT_EQ(
        MessageCodec::getMsgId(messageWithMsgId),
        1
    );
}


// 测试编码和解码
TEST(MessageCodecTest, EncodeDecode)
{
    json js;

    js["msgid"] = 1;
    js["username"] = "tom";
    js["password"] = "1234";

    string message = js.dump();

    string encoded =
        MessageCodec::encode(message);

    string decoded =
        MessageCodec::decode(encoded);

    EXPECT_EQ(decoded, message);
}


// 测试消息ID
TEST(MessageCodecTest, GetMsgId)
{
    json js;

    js["msgid"] = 123;
    js["message"] = "hello";

    string encoded =
        MessageCodec::encode(js.dump());

    // encode格式：
    // | totalLen | msgid | json |
    // getMsgId需要：
    // | msgid | json |
    string messageWithMsgId =
        encoded.substr(4);

    int msgid =
        MessageCodec::getMsgId(messageWithMsgId);

    EXPECT_EQ(msgid, 123);
}