#pragma once
#include <string>
#include <vector>

struct PrivateMessage{
    std::string from;
    std::string to;
    std::string message;
    std::string time;
};

class PrivateMessageModel{
    public:
      PrivateMessageModel();
      ~PrivateMessageModel();

      //保存私聊消息
      bool saveMessage(std::string from,std::string to,std::string message);
      //获取历史聊天记录
      std::vector<PrivateMessage> getMessages(const std::string& user1,const std::string& user2);
};
