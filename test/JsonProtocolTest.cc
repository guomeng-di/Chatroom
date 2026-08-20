#include <gtest/gtest.h>

#include "../protocol/JsonProtocol/JsonProtocol.h"

#include <string>

using namespace std;


// 测试 JSON 序列化
TEST(JsonProtocolTest, Encode)
{
    json js;

    js["msgid"] = 1;
    js["username"] = "tom";
    js["message"] = "hello";


    string result =
        JsonProtocol::encode(js);


    EXPECT_FALSE(result.empty());


    json expected = json::parse(result);


    EXPECT_EQ(
        expected["msgid"],
        1
    );

    EXPECT_EQ(
        expected["username"],
        "tom"
    );

    EXPECT_EQ(
        expected["message"],
        "hello"
    );
}


// 测试 JSON 反序列化
TEST(JsonProtocolTest, Decode)
{
    json js;

    js["msgid"] = 1;
    js["username"] = "tom";
    js["message"] = "hello";


    string jsonStr =
        JsonProtocol::encode(js);


    // JsonProtocol::decode()
    // 默认前4字节是长度
    //
    // | length(4) | json |
    //
    string packet(4, '\0');

    packet += jsonStr;


    json result =
        JsonProtocol::decode(packet);


    EXPECT_EQ(
        result["msgid"],
        1
    );

    EXPECT_EQ(
        result["username"],
        "tom"
    );

    EXPECT_EQ(
        result["message"],
        "hello"
    );
}


// 测试中文JSON
TEST(JsonProtocolTest, ChineseMessage)
{
    json js;

    js["msgid"] = 1;
    js["username"] = "tom";
    js["message"] = "你好，世界";


    string jsonStr =
        JsonProtocol::encode(js);


    string packet(4, '\0');

    packet += jsonStr;


    json result =
        JsonProtocol::decode(packet);


    EXPECT_EQ(
        result["message"],
        "你好，世界"
    );
}