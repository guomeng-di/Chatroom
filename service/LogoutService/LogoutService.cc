#include "LogoutService.h"
#include "../../protocol/MsgId.h"
#include "../../manager/RedisManager/RedisManager.h" 

#include "../FriendStatusService/FriendStatusService.h"
#include "../../manager/OnlineUserManager/OnlineUserManager.h"
#include "../../netlib/base/Logger.h"
using namespace std;

json LogoutService::logout(const json& js){
    json res;
    res["msgid"]=LOGOUT_ACK;
    if(!js.contains("username")){
        Logger::instance().error(
        "logout lack username"
    );
        res["errno"]=1,res["message"]="lack username";
        return res;
    }
    string username=js["username"];
    OnlineUserManager::instance().removeUser(username);
    //redis修改状态
    if(RedisManager::instance().connect()) RedisManager::instance().setOffline(username);

    Logger::instance().info(
        username + " remove online user success"
    );
    Logger::instance().info(
    username+" logout success"
);
    res["errno"]=0,res["message"]="logout success";
    FriendStatusService::notifyOffline(username);
    return res;
}