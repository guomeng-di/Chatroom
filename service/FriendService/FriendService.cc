#include "FriendService.h"
#include "../../model/FriendModel/FriendModel.h"
#include "../../model/FriendBlockModel/FriendBlockModel.h"
#include <iostream>
#include "../../model/UserModel/UserModel.h"
#include "../../protocol/MsgId.h"
//#include "../../manager/OnlineUserManager/OnlineUserManager.h" 
#include "../../manager/RedisManager/RedisManager.h" 
#include "../../client/FileClient/FileClient.h"
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

    //1. 参数检查
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
    //2. 不能添加自己
    if(username == friendName){
        response["errno"]=1;
        response["message"]="cannot add yourself";
        return response;
    }
    //检查好友用户是否存在
UserModel userModel;
if(!userModel.queryUserByUsername(friendName)){
    response["errno"]=1;
    response["message"]="user not exist";
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

    //4. 判断好友账号是否存在
    if(!userModel.queryUserByUsername(friendName)){
    response["errno"]=1;
    response["message"]="user not exist";
    return response;
}
    //5. 建立好友关系
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
        json friendInfo;
        friendInfo["name"]=f;
        friendInfo["online"]=RedisManager::instance().isOnline(f);
        friendInfo["online"]=RedisManager::instance().isOnline(f); 
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

        if(user==friendName){
        res["errno"]=1;
        res["message"]="cannot delete yourself";
        return res;
    }

        FriendModel model;
        if(!model.isFriend(user,friendName)){
            res["errno"]=1;
            res["message"]="not friends";
    return res;
}
    bool flag=model.removeFriend(user,friendName);

    cout<<"delete friend result="
    <<flag<<endl;


    if(flag){
        //删除好友之后
    //清理屏蔽关系
    FriendBlockModel blockModel;
    blockModel.removeAllBlock(user,friendName);
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
