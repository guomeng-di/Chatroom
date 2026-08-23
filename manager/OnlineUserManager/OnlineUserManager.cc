#include "OnlineUserManager.h"
#include "../../model/FriendModel/FriendModel.h"
#include <nlohmann/json.hpp>
#include "../../netlib/net/TcpConnection/TcpConnection.h"
#include <iostream>
#include "../../protocol/MsgId.h"
#include "../../service/FriendStatusService/FriendStatusService.h"
#include "../../protocol/MessageCodec/MessageCodec.h"
#include "../../protocol/JsonProtocol/JsonProtocol.h"
using namespace std;
OnlineUserManager::OnlineUserManager(){
}
OnlineUserManager& OnlineUserManager::instance(){
    static OnlineUserManager manager;
    return manager;
}
void OnlineUserManager::addUser(const std::string& username,TcpConnection* conn){
    users_[username]=conn;

    cout<<"add online user:"
        <<username
        <<endl;

}
bool OnlineUserManager::removeUser(const std::string& username, TcpConnection* conn){
    cout<<"========== OnlineUserManager::removeUser =========="<<endl;
    cout<<"username="<<username<<endl;
    cout<<"isOnline="<<isOnline(username)<<endl;
    if(!isOnline(username)){
        cout<<"user is already offline"<<endl;
        return false;
    }
    auto it = users_.find(username);
    // 同一用户可能已经重新登录，旧连接关闭时不能删除新连接。
    if(conn != nullptr && it != users_.end() && it->second != conn){
        cout<<"user connection already replaced, keep current online entry"<<endl;
        return false;
    }
    users_.erase(username);
    cout<<"user erased from online map: "<<username<<endl;
    FriendStatusService::notifyOffline(username);
    return true;
}
TcpConnection* OnlineUserManager::getConnection(const std::string& username){
    auto it=users_.find(username);
    //if(it!=users_.end()) return it->second;
    if(it!=users_.end())
    {
        cout<<"find online user:"
            <<username
            <<" success"
            <<endl;

        return it->second;
    }


    cout<<"find online user:"
        <<username
        <<" failed"
        <<endl;

    return NULL;
}
bool OnlineUserManager::isOnline(const std::string& username){
    return users_.find(username)!=users_.end();
}
