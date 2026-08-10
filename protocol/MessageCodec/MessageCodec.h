//负责消息在网络上传输时的编码和解码。
// 发送方：JSON → 加长度 → 字节流
// 接收方：字节流 → 去长度 → JSON
#pragma once
#include <string>

class MessageCodec{
    public:
      MessageCodec();
      ~MessageCodec();
      //编码：消息 -> 长度+消息
      static std::string encode(const std::string& msg);
      //解码：长度+消息 -> 消息
      static std::string decode(const std::string& data);
};