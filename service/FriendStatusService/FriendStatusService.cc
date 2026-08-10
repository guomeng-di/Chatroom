#include "FriendStatusService.h"

#include "../../model/FriendModel/FriendModel.h"
#include "../../manager/OnlineUserManager/OnlineUserManager.h"
#include "../../netlib/net/TcpConnection/TcpConnection.h"
#include "../../protocol/MsgId.h"
#include "../../protocol/MessageCodec/MessageCodec.h"
#include <iostream>
#include <unordered_set>
using namespace std;
extern OnlineUserManager onlineUserManager;
// 用户下线通知
void FriendStatusService::notifyOffline(const string& username){
    //if(!isOnline(username)) return;
    //获取该用户所有好友
    FriendModel friendModel;
    unordered_set<string> friendList=friendModel.getFriends(username);
    //先从在线表移除用户
    //users_.erase(username);
    cout<<"notify offline:"<<username<<endl;
    //组装下线通知json
    json js;
    js["msgid"]=FRIEND_STATUS_NOTIFY;
    js["username"]=username;
    js["online"]=false;
    js["message"]="user offline";
    //序列化
    string jsonStr=js.dump();
    //string sendBuf=MessageCodec::encode(jsonStr);
    //遍历好友,推送下线通知
    for(const string& friendName:friendList){
        TcpConnection* conn=onlineUserManager.getConnection(friendName);
        if(conn!=NULL){
            cout<<"send offline notify to "<<friendName<<endl;
             conn->send(jsonStr);
        }
    }
}


// 用户上线通知
void FriendStatusService::notifyOnline(const string& username){
    //if(!isOnline(username)) return;
    //获取该用户所有好友
    FriendModel friendModel;
    unordered_set<string> friendList=friendModel.getFriends(username);
    //先从在线表移除用户
    //users_.erase(username);
    //cout<<"notify offline:"<<username<<endl;
    //组装下线通知json
    json js;
    js["msgid"]=FRIEND_STATUS_NOTIFY;
    js["username"]=username;
    js["online"]=true;
    js["message"]="user online";
    //序列化
    string jsonStr=js.dump();
    //string sendBuf=MessageCodec::encode(jsonStr);
    //遍历好友,推送下线通知
    for(const string& friendName:friendList){
        TcpConnection* conn=onlineUserManager.getConnection(friendName);
        if(conn!=NULL){
            cout<<"send online notify to "<<friendName<<endl;
             conn->send(jsonStr);
        }
    }

}