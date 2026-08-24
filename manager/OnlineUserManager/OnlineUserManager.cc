#include "OnlineUserManager.h"
#include "../../model/FriendModel/FriendModel.h"
#include <nlohmann/json.hpp>
#include "../../netlib/net/TcpConnection/TcpConnection.h"
#include "../../netlib/base/Logger/Logger.h"
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

    LOG_INFO<<"添加在线用户:"<<username;

}
bool OnlineUserManager::removeUser(const std::string& username, TcpConnection* conn){
    LOG_INFO<<"========== OnlineUserManager::removeUser ==========";
    LOG_INFO<<"用户名:"<<username;
    LOG_INFO<<"当前在线状态:"<<isOnline(username);
    if(!isOnline(username)){
        LOG_INFO<<"用户已经处于离线状态";
        return false;
    }
    auto it = users_.find(username);
    // 同一用户可能已经重新登录，旧连接关闭时不能删除新连接。
    if(conn != nullptr && it != users_.end() && it->second != conn){
        LOG_INFO<<"用户连接已经被替换,保持当前在线连接";
        return false;
    }
    users_.erase(username);
    LOG_INFO<<"从在线用户列表删除用户:"<<username;
    FriendStatusService::notifyOffline(username);
    return true;
}
TcpConnection* OnlineUserManager::getConnection(const std::string& username){
    auto it=users_.find(username);
    //if(it!=users_.end()) return it->second;
    if(it!=users_.end()){
        LOG_INFO<<"查找在线用户:"<<username<<"成功";

        return it->second;
    }

    LOG_INFO<<"查找在线用户:"<<username<<"失败";

    return NULL;
}
bool OnlineUserManager::isOnline(const std::string& username){
    return users_.find(username)!=users_.end();
}