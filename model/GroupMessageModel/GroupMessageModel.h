#pragma once
#include <string>
#include <vector>


//群消息结构体
struct GroupMessage{
    std::string groupname;   //群名称
    std::string from;        //发送者
    std::string message;     //消息内容
    std::string time;        //发送时间
};


class GroupMessageModel{
public:
    GroupMessageModel();
    ~GroupMessageModel();

    //保存群消息
    bool saveMessage(const std::string& groupname,const std::string& from,const std::string& message);
    //查询群历史消息
    std::vector<GroupMessage> getMessages(const std::string& groupname);

};