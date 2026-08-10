//处理用户之间的消息转发
#pragma once
#include <nlohmann/json.hpp>
class TcpConnection;
using json=nlohmann::json;
class ChatService{
    public:
      ChatService();
      ~ChatService();

      static json chat(const json& js,TcpConnection* conn);
};