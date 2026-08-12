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
void OnlineUserManager::removeUser(const std::string& username){
    if(!isOnline(username)) return;
    //先从在线表移除用户
    users_.erase(username);
    FriendStatusService::notifyOffline(username);
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