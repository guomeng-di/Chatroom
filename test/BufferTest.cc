#include <gtest/gtest.h>
#include "../netlib/net/Buffer/Buffer.h"
#include <arpa/inet.h>
#include <string>
using namespace std;

// 测试完整消息
TEST(BufferTest, CompleteMessage){
    Buffer buffer;
    string message = "hello";
    uint32_t len = htonl(message.size());
    // 添加4字节消息长度
    buffer.append(reinterpret_cast<const char*>(&len),sizeof(len));
    // 添加消息正文
    buffer.append(message.data(),message.size());
    // 应该能够检测到完整消息
    EXPECT_TRUE(buffer.hasMessage());
    // 取出消息
    string result = buffer.retrieveMessage();
    EXPECT_EQ(result, message);
}

// 测试半包
TEST(BufferTest, HalfMessage){
    Buffer buffer;
    string message = "hello";
    uint32_t len = htonl(message.size());
    // 只发送消息长度
    buffer.append(reinterpret_cast<const char*>(&len),sizeof(len));
    // 此时没有完整消息
    EXPECT_FALSE(buffer.hasMessage());

    // 再发送消息正文
    buffer.append(
        message.data(),
        message.size()
    );

    // 现在应该有完整消息
    EXPECT_TRUE(buffer.hasMessage());

    string result = buffer.retrieveMessage();

    EXPECT_EQ(result, message);
}
// 测试两个连续消息
TEST(BufferTest, MultipleMessages)
{
    Buffer buffer;

    string message1 = "hello";
    string message2 = "world";


    uint32_t len1 = htonl(message1.size());
    uint32_t len2 = htonl(message2.size());


    // 第一个消息
    buffer.append(
        reinterpret_cast<const char*>(&len1),
        sizeof(len1)
    );

    buffer.append(
        message1.data(),
        message1.size()
    );


    // 第二个消息
    buffer.append(
        reinterpret_cast<const char*>(&len2),
        sizeof(len2)
    );

    buffer.append(
        message2.data(),
        message2.size()
    );


    // 第一个
    EXPECT_TRUE(buffer.hasMessage());

    string result1 = buffer.retrieveMessage();

    EXPECT_EQ(result1, message1);


    // 第二个
    EXPECT_TRUE(buffer.hasMessage());

    string result2 = buffer.retrieveMessage();

    EXPECT_EQ(result2, message2);
}

TEST(BufferTest, OversizedMessage)
{
    Buffer buffer;

    uint32_t len =
        htonl(10 * 1024 * 1024 + 1);

    buffer.append(
        reinterpret_cast<const char*>(&len),
        sizeof(len)
    );

    EXPECT_FALSE(buffer.hasMessage());

    EXPECT_TRUE(buffer.hasError());
}