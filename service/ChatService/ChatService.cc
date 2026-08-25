#include "ChatService.h"
#include "../../manager/OnlineUserManager/OnlineUserManager.h" 
#include "../../manager/RedisManager/RedisManager.h" 
#include "../../manager/FriendManager/FriendManager.h" 

#include "../../model/PrivateMessageModel/PrivateMessageModel.h"
#include "../../netlib/net/TcpConnection/TcpConnection.h"
// #include "../../model/FriendModel/FriendModel.h"
// #include "../../model/FriendBlockModel/FriendBlockModel.h"
#include "../../protocol/MsgId.h"
#include <iostream>
#include "../../netlib/base/Logger/Logger.h"
#include "../../netlib/base/TaskThreadPool/TaskThreadPool.h"

using namespace std;
ChatService::ChatService(){}
ChatService::~ChatService(){}
json ChatService::chat(const json& js,TcpConnection* conn){
    //1知道from,to是谁
    json response;
     if(!js.contains("from")||!js.contains("to")||!js.contains("message")){
        //cout<<"chat parameter error"<<endl;
        response["msgid"]=CHAT_ACK;
        response["errno"]=1;
        response["message"]="parameter error";
        LOG_ERROR<<"聊天参数错误";
        return response;
    }
    // string from=js["from"];
    string from=conn->getUsername();
    string to=js["to"];
    //2知道message
    string msg=js["message"];

    LOG_INFO<<"收到私聊请求 from="<<from<<" to="<<to;

    if(from==to){
        response["msgid"]=CHAT_ACK;
        response["errno"]=1;
        response["message"]="cannot chat yourself";
        LOG_WARN<<"用户尝试给自己发送消息 username="<<from;
        return response;
}
    //判断是否好友
if(!FriendManager::instance().isFriend(from,to)){

        LOG_ERROR<<"双方不是好友 from="
                 <<from
                 <<" to="
                 <<to;


        response["msgid"]=CHAT_ACK;
        response["errno"]=1;
        response["message"]="not friend";

        return response;
    }
    //判断屏蔽了没
    if(FriendManager::instance().isBlocked(to,from)){
    response["msgid"]=CHAT_ACK;
    response["errno"]=1;
    response["message"]="you are blocked";

    LOG_WARN<<"用户被屏蔽 from="
            <<from
            <<" to="
            <<to;

    return response;
}
    //发送信息
    json sendMsg;
        sendMsg["msgid"]=CHAT_NOTIFY;
        sendMsg["from"]=from;
        sendMsg["message"]=msg;
        sendMsg["to"]=to;
        TcpConnection* target=OnlineUserManager::instance().getConnection(to);
        if(target){
            target->send(sendMsg.dump());
        }

        //在线消息先完成网络转发，历史记录异步保存，避免阻塞事件循环
        if(target){
            TaskThreadPool::instance().enqueue([from,to,msg](){
                PrivateMessageModel model;
                if(!model.saveMessage(from,to,msg)){
                    LOG_ERROR<<"保存私聊消息失败 from="<<from<<" to="<<to;
                }
                else{
                    LOG_INFO<<"保存私聊消息成功 from="<<from<<" to="<<to;
                }
            });
        }else{
            //离线消息保持原有同步保存逻辑
            PrivateMessageModel model;
            if(!model.saveMessage(from,to,msg)){
                LOG_ERROR<<"保存私聊消息失败 from="<<from<<" to="<<to;
            }
            else{
                LOG_INFO<<"保存私聊消息成功 from="<<from<<" to="<<to;
            }
        }

    //4调用 TcpConnection::send() 转发
    if(target){
        LOG_INFO<<"发送在线私聊消息 from="<<from<<" to="<<to;
        response["msgid"]=CHAT_ACK;
        response["errno"]=0;
        response["message"]="send success";
    }else{
        //离线->redis
        RedisManager::instance().connect();
        string offlineMsg = sendMsg.dump();

        LOG_INFO<<"用户离线,保存离线消息 username="<<to;
    
        RedisManager::instance().saveOfflineMessage(to,offlineMsg);
        response["msgid"]=CHAT_ACK;
        response["errno"]=0;
        response["message"]="user offline, save message";
    }
    //5. 回复发送者
     return response;
}
