#include "LogoutService.h"
#include "../../protocol/MsgId.h"
#include "../../manager/RedisManager/RedisManager.h" 
#include "../FriendStatusService/FriendStatusService.h"
#include "../../manager/OnlineUserManager/OnlineUserManager.h"
#include "../../netlib/base/Logger/Logger.h"
using namespace std;

json LogoutService::logout(const json& js){
    json res;
    res["msgid"]=LOGOUT_ACK;

    if(!js.contains("username")){
        LOG_ERROR<<"用户注销失败，缺少用户名参数";
        res["errno"]=1;
        res["message"]="lack username";
        return res;
    }

    string username=js["username"];
    LOG_INFO<<"用户请求注销，用户名:"<<username;
    OnlineUserManager::instance().removeUser(username);
    LOG_INFO<<"移除在线用户成功，用户名:"<<username;

    if(RedisManager::instance().connect()){
        RedisManager::instance().setOffline(username);
        LOG_INFO<<"更新用户离线状态成功，用户名:"<<username;
    }else{
        LOG_ERROR<<"Redis连接失败，无法更新用户离线状态，用户名:"<<username;
    }

    LOG_INFO<<"用户注销成功，用户名:"<<username;

    res["errno"]=0;
    res["message"]="logout success";

    return res;
}