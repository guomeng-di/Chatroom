#include "FriendService.h"
#include "../../model/FriendModel/FriendModel.h"
#include <iostream>
#include "../../protocol/MsgId.h"
#include "../../manager/OnlineUserManager/OnlineUserManager.h" 
#include "../../manager/RedisManager/RedisManager.h" 

#include "../../netlib/base/Logger.h"
using namespace std;
     //OnlineUserManager onlineUserManager;

FriendService::FriendService(){
}
FriendService::~FriendService(){
}
json FriendService::addFriend(const json& js){
    json response;
    response["msgid"]=ADD_FRIEND_ACK;

    if(!js.contains("username") || !js.contains("friendname")){
        Logger::instance().error("add friend lack params");
        response["errno"]=1;
        response["message"]="lack params";
        return response;
    }
    string username=js["username"];
    string friendName=js["friendname"];
    if(username.empty() || friendName.empty()){
        response["errno"]=1;
        response["message"]="username/friendname cannot empty";
        return response;
    }
    if(username == friendName){
        response["errno"]=1;
        response["message"]="cannot add yourself";
        return response;
    }
    FriendModel model;
    //先判断是否已经是好友
    if(model.isFriend(username,friendName)){
        Logger::instance().error( username+" and "+friendName+" already friends");
        response["errno"]=1;
        response["message"]="already friends";
        return response;
    }

    bool flag=model.addFriend(username,friendName);
    if(flag){
        Logger::instance().info(username+" add friend "+friendName+" success");
        response["errno"]=0;
        response["message"]="add friend success";
    }else{
        Logger::instance().error(username+" add friend "+friendName+" failed");
        response["errno"]=1;
        response["message"]="add friend fail";
    }
    return response;
}
json FriendService::getFriendList(const json& js){
    json res;
    res["msgid"]=FRIEND_LIST_ACK;
    res["friends"]=json::array();
    if(!js.contains("username")){
        Logger::instance().error("get friend list lack username");
        res["errno"]=1;
        res["message"]="lack username";
        return res;
    }
    string user=js["username"];
    FriendModel model;
    unordered_set<string> friends=model.getFriends(user);
    res["errno"]=0;
    // OnlineUserManager onlineUserManager;
    for(auto& f:friends){
        RedisManager redis;
        json friendInfo;
        friendInfo["name"]=f;
        friendInfo["online"]=redis.isOnline(f);
        friendInfo["online"]=redis.isOnline(f); 
        res["friends"].push_back(friendInfo);
    }
    return res;
}
json FriendService::deleteFriend(const json& js){
    json res;
    res["msgid"]=DELETE_FRIEND_ACK;
    if(!js.contains("username")||!js.contains("friendname")){
        Logger::instance().error("delete friend lack params");
        res["errno"]=1;
        res["message"]="lack username";
        return res;
    }
        string user=js["username"];
        string friendName=js["friendname"];

    FriendModel model;
    bool flag=model.removeFriend(user,friendName);
    if(flag){
        Logger::instance().info(user+" delete friend "+friendName+" success");
    res["errno"]=0;
    res["message"]="delete friend success";
}
else{
    Logger::instance().error(user+" delete friend "+friendName+" failed");
    res["errno"]=1;
    res["message"]="delete friend fail";
}
    return res;
}
