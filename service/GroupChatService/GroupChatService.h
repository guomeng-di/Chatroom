#pragma once
#include <nlohmann/json.hpp>
class TcpConnection;
using json=nlohmann::json;
class GroupChatService{
    public:
      GroupChatService();
      ~GroupChatService();

      static json groupChat(const json& js,TcpConnection* conn);
};
//conn告诉我是谁发的，js告诉我发什么内容，我根据群成员列表找到其他conn，把消息广播出去