//负责消息在网络上传输时的编码和解码。
// 发送方：JSON → 加长度 → 字节流
// 接收方：字节流 → 去长度 → JSON
#pragma once
#include <string>
#include <nlohmann/json.hpp>
using json=nlohmann::json;

struct FilePacket{
  int msgid;
  json info;
  std::string data;
};//把服务器收到的消息拆开
class MessageCodec{
    public:
      MessageCodec();
      ~MessageCodec();
      //编码：消息 -> 长度+消息
      static std::string encode(const std::string& msg);
      //解码：长度+消息 -> 消息
      static std::string decode(const std::string& data);
      //二进制消息
      static std::string encodeBinary(int msgid,const json& js,const std::string& data);
      static bool decodeHeader(const std::string& data,int& msgid,int& bodyLen);

      static FilePacket decodeBinary(const std::string& msg);
      static int getMsgId(const std::string& data);
};