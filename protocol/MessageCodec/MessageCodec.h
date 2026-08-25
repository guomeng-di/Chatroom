//负责消息在网络上传输时的编码(加长度)和解码(去除长度)
// 发送方：JSON → 加长度 → 字节流
// 接收方：字节流 → 去长度 → JSON
#pragma once
#include <string>
#include <nlohmann/json.hpp>
using json=nlohmann::json;

//把服务器收到的消息拆开
struct FilePacket{
  int msgid;//msgid
  json info;//json
  std::string data;//内容
};
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
      static std::string encodeBinary(int msgid,const json& js,const char* data,size_t size);
      static FilePacket decodeBinary(const std::string& msg);
      //static bool decodeHeader(const std::string& data,int& msgid,int& bodyLen);
      static int getMsgId(const std::string& data);
};
