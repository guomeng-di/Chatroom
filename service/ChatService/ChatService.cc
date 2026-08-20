#include "ChatService.h"
#include "../../manager/OnlineUserManager/OnlineUserManager.h" 
#include "../../manager/RedisManager/RedisManager.h" 
#include "../../model/PrivateMessageModel/PrivateMessageModel.h"
#include "../../netlib/net/TcpConnection/TcpConnection.h"
#include "../../model/FriendModel/FriendModel.h"
#include "../../model/FriendBlockModel/FriendBlockModel.h"
//#include "../../model/OfflineMessageModel/OfflineMessageModel.h"
#include "../../protocol/MsgId.h"
#include <iostream>
#include "../../netlib/base/Logger.h"

using namespace std;
ChatService::ChatService(){}
ChatService::~ChatService(){}
json ChatService::chat(const json& js,TcpConnection* conn){
    //1知道from,to是谁
    json response;
     if(!js.contains("from")||!js.contains("to")||!js.contains("message")){
        //cout<<"chat parameter error"<<endl;
        Logger::instance().error(
        "chat parameter error"
    );
        return response;
    }
    // string from=js["from"];
    string from=conn->getUsername();
    string to=js["to"];
    //2知道message
    string msg=js["message"];

    if(from==to){
        response["message"]="cannot chat yourself";
        return response;
}
    //判断是否好友
FriendModel friendModel;
if(!friendModel.isFriend(from,to)){
    //cout<<"not friend"<<endl;
    Logger::instance().error(from+" and "+to+" are not friends" );
    response["msgid"]=CHAT_ACK;
    response["errno"]=1;
    response["message"]="not friend";
    return response;
}
    //判断屏蔽了没
    FriendBlockModel blockModel;
    if(blockModel.isBlocked(to,from)){
    response["msgid"]=CHAT_ACK;
    response["errno"]=1;
    response["message"]="you are blocked";
    //conn->send(response.dump());
    return response;
}
    //发送信息
    json sendMsg;
        sendMsg["msgid"]=CHAT_NOTIFY;
        sendMsg["from"]=from;
        sendMsg["message"]=msg;
        //先存入mysql
        PrivateMessageModel model;
        if(!model.saveMessage(from,to,msg))
            Logger::instance().error("save private message failed");
        //model.saveMessage(from,to,msg);

            TcpConnection* target=OnlineUserManager::instance().getConnection(to);
    //4调用 TcpConnection::send() 转发
    if(target){
        target->send(sendMsg.dump());
        Logger::instance().info(from+" send message to "+to);
        response["msgid"]=CHAT_ACK;
        response["errno"]=0;
        response["message"]="send success";
    }else{
        //离线->redis
        RedisManager::instance().connect();
        string offlineMsg = sendMsg.dump();

        Logger::instance().info(to+" is offline, save message");
    
        RedisManager::instance().saveOfflineMessage(to,offlineMsg);
        response["msgid"]=CHAT_ACK;
        response["errno"]=0;
        response["message"]="user offline, save message";
    }
    //5. 回复发送者
     return response;
}