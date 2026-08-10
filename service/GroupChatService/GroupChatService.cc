#include "GroupChatService.h"
#include "../../model/GroupModel/GroupModel.h"
#include "../../manager/OnlineUserManager/OnlineUserManager.h"
#include "../../manager/RedisManager/RedisManager.h"
#include "../../model/GroupMessageModel/GroupMessageModel.h"
#include "../../netlib/net/TcpConnection/TcpConnection.h"
#include <iostream>
#include "../../protocol/MsgId.h"
#include "../../netlib/base/Logger.h"
using namespace std;
// {
//             msgid:,
//             groupName:"cpp",
//             username:"tom",
//             message:"hello"
//         }
// 在线用户管理器
extern OnlineUserManager onlineUserManager;
GroupChatService::GroupChatService(){}
GroupChatService::~GroupChatService(){}
json GroupChatService::groupChat(const json& js,TcpConnection* conn){
    json response;
    response["msgid"]=GROUP_CHAT_ACK;
    if(!js.contains("groupname")||!js.contains("from")||!js.contains("message")){
        Logger::instance().error(
        "group chat lack params"
    );
        response["errno"]=1,response["message"]="lack params";
        return response;
    }
    string groupName=js["groupname"];
    string username=js["from"];
    string message=js["message"];
    if(groupName.empty()||username.empty()||message.empty()){
        Logger::instance().error(
        "group chat params empty"
    );
        response["errno"]=1,response["message"]="params cannot empty";
        return response;
    }
    GroupModel groupModel;
    if(!groupModel.groupExist(groupName)){
         Logger::instance().error(
        username+
        " send message to group "+
        groupName+
        " but group not exist"
    );
        response["errno"]=1,response["message"]="group not exist";
        return response;
    }//判断群是否存在
    if(!groupModel.isMember(groupName,username)){
        Logger::instance().error(
        username+
        " is not member of group "+
        groupName
    );
        response["errno"]=1,response["message"]="not group member";
        return response;
    }//判断发送者是不是群成员

    // 保存历史消息
    GroupMessageModel messageModel;
    messageModel.saveMessage(groupName, username, message);


    //获取群成员
    unordered_set<string> members=groupModel.getMembers(groupName);
    //构造群消息
    json sendMsg;
    sendMsg["msgid"]=GROUP_CHAT_NOTIFY;
    sendMsg["groupname"]=groupName;
    sendMsg["from"]=username;
    sendMsg["message"]=message;

    string data=sendMsg.dump();
    
    int sendCount=0,offlineCount=0;
    //遍历群成员发送
    for(auto& member:members){
        if(member==username) continue;
        TcpConnection* target=onlineUserManager.getConnection(member);
    //在线
    if(target){
        target->send(data);
        sendCount++;
        //cout<<"send group message to online user:"<<member<<endl;
    }
    //离线
    else{
        //cout<<"save group offline message:"<<member<<endl;
        //Redis保存离线消息需要
        RedisManager redis;
        if(redis.connect()){
        redis.saveGroupOfflineMessage(member,data);
        offlineCount++;
    }
    }
}
    Logger::instance().info(
    username+
    " send group message group="+
    groupName+
    " online="+
    to_string(sendCount)+
    " offline="+
    to_string(offlineCount)
);
    //回复发送者
    response["errno"]=0;
    response["message"]="group send success";
    return response;
}