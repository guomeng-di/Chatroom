#include <gtest/gtest.h>

#include "../utils/SHA256/SHA256.h"

#include <string>

using namespace std;


// 测试固定字符串
TEST(SHA256Test, Encode)
{
    string input = "hello";

    string result =
        HashSHA256::encode(input);


    EXPECT_EQ(
        result,
        "2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824"
    );
}


// 测试空字符串
TEST(SHA256Test, EmptyString)
{
    string input = "";

    string result =
        HashSHA256::encode(input);


    EXPECT_EQ(
        result,
        "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    );
}


// 相同输入应该得到相同结果
TEST(SHA256Test, SameInputSameHash)
{
    string input = "123456";

    string hash1 =
        HashSHA256::encode(input);

    string hash2 =
        HashSHA256::encode(input);


    EXPECT_EQ(hash1, hash2);
}


// 不同输入应该得到不同结果
TEST(SHA256Test, DifferentInputDifferentHash)
{
    string input1 = "123456";
    string input2 = "123457";


    string hash1 =
        HashSHA256::encode(input1);

    string hash2 =
        HashSHA256::encode(input2);


    EXPECT_NE(hash1, hash2);
}


// SHA256结果应该是64个十六进制字符
TEST(SHA256Test, HashLength)
{
    string input = "hello";

    string result =
        HashSHA256::encode(input);


    EXPECT_EQ(result.size(), 64);
}