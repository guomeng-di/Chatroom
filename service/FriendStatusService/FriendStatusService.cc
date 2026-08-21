#include "FriendStatusService.h"

#include "../../model/FriendModel/FriendModel.h"
#include "../../manager/OnlineUserManager/OnlineUserManager.h"
#include "../../netlib/net/TcpConnection/TcpConnection.h"
#include "../../protocol/MsgId.h"
#include "../../protocol/MessageCodec/MessageCodec.h"
#include <iostream>
#include <unordered_set>
using namespace std;
// 用户下线通知
void FriendStatusService::notifyOffline(const string& username){
    cout<<"notifyOffline called username="
    <<username<<endl;
    cout<<"notify offline:"<<username<<endl;
    //获取该用户所有好友
    FriendModel friendModel;
    unordered_set<string> friendList=friendModel.getFriends(username);
    //组装下线通知json
    json js;
    js["msgid"]=FRIEND_STATUS_NOTIFY;
    js["username"]=username;
    js["online"]=false;
    //序列化
    string jsonStr=js.dump();
    //遍历好友,推送下线通知
    for(const string& friendName:friendList){
        TcpConnection* conn=OnlineUserManager::instance().getConnection(friendName);
        if(conn!=NULL){
            cout<<"send offline notify to "<<friendName<<endl;
             conn->send(jsonStr);
        }
    }
}


// 用户上线通知
void FriendStatusService::notifyOnline(const string& username){
    cout<<"notify online:"<<username<<endl;
    //1获取好友列表
    FriendModel friendModel;
    unordered_set<string> friendList=friendModel.getFriends(username);
    //2构造通知消息
    json js;
    js["msgid"]=FRIEND_STATUS_NOTIFY;
    js["username"]=username;
    js["online"]=true;
    string jsonStr=js.dump();

   //3通知在线好友
    for(const string& friendName:friendList){
        TcpConnection* conn=OnlineUserManager::instance().getConnection(friendName);
        //好友在线时通知
        if(conn){
            cout<<"send online notify to "<<friendName<<endl;
            conn->send(jsonStr);
        }
    }

}