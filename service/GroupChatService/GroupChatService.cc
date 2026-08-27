#include "GroupChatService.h"
#include "../../model/GroupModel/GroupModel.h"
#include "../../manager/OnlineUserManager/OnlineUserManager.h"
#include "../../manager/RedisManager/RedisManager.h"
#include "../../model/GroupMessageModel/GroupMessageModel.h"
#include "../../netlib/net/TcpConnection/TcpConnection.h"
#include <iostream>
#include "../../protocol/MsgId.h"
#include "../../netlib/base/Logger/Logger.h"
#include "../../netlib/base/TaskThreadPool/TaskThreadPool.h"
using namespace std;
GroupChatService::GroupChatService(){}
GroupChatService::~GroupChatService(){}
json GroupChatService::groupChat(const json& js,TcpConnection* conn){
    json response;
    response["msgid"]=GROUP_CHAT_ACK;
    if(!js.contains("groupname")||!js.contains("message")){
        LOG_ERROR<<"群聊请求缺少参数";
        response["errno"]=1,response["message"]="群聊请求缺少参数";
        return response;
    }
    string groupName=js["groupname"];
    string username=conn->getUsername();
    string message=js["message"];
    if(groupName.empty()||username.empty()||message.empty()){
        LOG_ERROR<<"群聊参数为空";
        response["errno"]=1,response["message"]="输入不可为空";
        return response;
    }
    GroupModel groupModel;
    if(!groupModel.groupExist(groupName)){
        LOG_ERROR<<username<<"发送群消息失败，群不存在:"<<groupName;
        response["errno"]=1,response["message"]="发送群消息失败，群不存在";
        return response;
    }
    if(!groupModel.isMember(groupName,username)){
        LOG_ERROR<<username<<"不是群成员，无法发送群消息:"<<groupName;
        response["errno"]=1,response["message"]="不是群成员，无法发送群消息";
        return response;
    }
    unordered_set<string> members=groupModel.getMembers(groupName);
    if(members.empty()){
        LOG_ERROR<<"获取群成员失败，无法发送群聊消息";
        response["errno"]=1;
        response["message"]="获取群成员失败，无法发送群聊消息";
        return response;
    }
    json sendMsg;
    sendMsg["msgid"]=GROUP_CHAT_NOTIFY;
    sendMsg["groupname"]=groupName;
    sendMsg["from"]=username;
    sendMsg["message"]=message;
    string data=sendMsg.dump();
    int sendCount=0,offlineCount=0;
    unordered_set<string> offlineMembers;
    for(auto& member:members){
        if(member==username){
            continue;
        }
        TcpConnection* target=OnlineUserManager::instance().getConnection(member);
        if(target){
            target->send(data);
            sendCount++;
        }else{
            offlineMembers.insert(member);
        }
    }
    //群聊通知先完成网络发送，历史记录异步保存，避免阻塞事件循环
    TaskThreadPool::instance().enqueue([groupName,username,message](){
        GroupMessageModel messageModel;
        if(!messageModel.saveMessage(groupName,username,message)){
            LOG_ERROR<<"保存群聊历史消息失败";
        }
    });
    if(!offlineMembers.empty()&&RedisManager::instance().connect()){
        for(auto& member:offlineMembers){
            if(RedisManager::instance().saveGroupOfflineMessage(member,data)){
                offlineCount++;
            }
        }
    }
    LOG_INFO<<username<<"发送群消息成功，群:"<<groupName<<" 在线人数:"<<sendCount<<" 离线人数:"<<offlineCount;
    response["errno"]=0;
    response["message"]="group send success";
    return response;
}
