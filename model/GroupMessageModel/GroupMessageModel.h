#pragma once
#include <string>
#include <vector>
//群消息结构体
struct GroupMessage{
    long long id;
    std::string groupname;
    std::string from;
    std::string message;
    std::string time;
};
class GroupMessageModel{
public:
    GroupMessageModel();
    ~GroupMessageModel();
    //保存群消息
    bool saveMessage(const std::string& groupname,const std::string& from,const std::string& message);
    //查询群历史消息
    std::vector<GroupMessage> getMessages(const std::string& groupname,long long beforeId=0);
};