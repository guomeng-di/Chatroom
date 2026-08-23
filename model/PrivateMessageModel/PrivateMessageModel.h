#pragma once
#include <string>
#include <vector>

struct PrivateMessage{
    long long id;
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
      //获取历史聊天记录->beforeId为0时获取最新50条，否则获取指定id之前的50条
      std::vector<PrivateMessage> getMessages(const std::string& user1,const std::string& user2,long long beforeId=0);
};
