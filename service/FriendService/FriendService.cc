#include "FriendService.h"
#include "../../model/FriendModel/FriendModel.h"
#include "../../model/FriendBlockModel/FriendBlockModel.h"
#include "../../model/UserModel/UserModel.h"
#include "../../protocol/MsgId.h"
#include "../../manager/RedisManager/RedisManager.h"
#include "../../netlib/base/Logger/Logger.h"

using namespace std;
FriendService::FriendService(){
}
FriendService::~FriendService(){
}
json FriendService::addFriend(const json& js){
    LOG_INFO<<"收到添加好友请求";
    json response;
    response["msgid"]=ADD_FRIEND_ACK;
    //1. 参数检查
    if(!js.contains("username")||!js.contains("friendname")){
        LOG_ERROR<<"添加好友请求缺少参数";
        response["errno"]=1;
        response["message"]="添加好友请求缺少参数";
        return response;
    }
    string username=js["username"];
    string friendName=js["friendname"];
    if(username.empty() || friendName.empty()){
        LOG_ERROR<<"用户名或好友名为空";
        response["errno"]=1;
        response["message"]="用户名或好友名为空";
        return response;
    }
    //2. 不能添加自己
    if(username == friendName){
        LOG_ERROR<<"不能添加自己为好友";
        response["errno"]=1;
        response["message"]="不能添加自己为好友";
        return response;
    }
    //检查好友用户是否存在
    UserModel userModel;
    if(!userModel.queryUserByUsername(friendName)){
        LOG_ERROR<<"用户不存在:"<<friendName;
        response["errno"]=1;
        response["message"]="用户不存在";
        return response;
    }
    FriendModel model;
    //先判断是否已经是好友
    if(model.isFriend(username,friendName)){
        LOG_ERROR<<username<<"和"<<friendName<<"已经是好友";
        response["errno"]=1;
        response["message"]="已经是好友";
        return response;
    }
    //5. 建立好友关系
    bool flag=model.addFriend(username,friendName);
    if(flag){
        LOG_INFO<<username<<"添加好友成功:"<<friendName;
        response["errno"]=0;
        response["message"]="添加好友成功";
    }else{
        LOG_ERROR<<username<<"添加好友失败:"<<friendName;
        response["errno"]=1;
        response["message"]="添加好友成功";
    }
    return response;
}
json FriendService::getFriendList(const json& js){
    LOG_INFO<<"收到获取好友列表请求";
    json res;
    res["msgid"]=FRIEND_LIST_ACK;
    res["friends"]=json::array();
    if(!js.contains("username")){
        LOG_ERROR<<"获取好友列表缺少用户名";
        res["errno"]=1;
        res["message"]="获取好友列表缺少用户名";
        return res;
    }
    string user=js["username"];
    FriendModel model;
    unordered_set<string> friends=model.getFriends(user);
    res["errno"]=0;
    for(auto& f:friends){
        json friendInfo;
        friendInfo["name"]=f;
        friendInfo["online"]=RedisManager::instance().isOnline(f);
        res["friends"].push_back(friendInfo);
    }
    LOG_INFO<<"获取好友列表成功:"<<user;
    return res;
}
json FriendService::deleteFriend(const json& js){
    LOG_INFO<<"收到删除好友请求";
    json res;
    res["msgid"]=DELETE_FRIEND_ACK;
    if(!js.contains("username")||!js.contains("friendname")){
        LOG_ERROR<<"删除好友请求缺少参数";
        res["errno"]=1;
        res["message"]="删除好友请求缺少参数";
        return res;
    }
    string user=js["username"];
    string friendName=js["friendname"];
    if(user==friendName){
        LOG_ERROR<<"不能删除自己";
        res["errno"]=1;
        res["message"]="不能删除自己";
        return res;
    }
    FriendModel model;
    if(!model.isFriend(user,friendName)){
        LOG_ERROR<<user<<"和"<<friendName<<"不是好友";
        res["errno"]=1;
        res["message"]="不是好友";
        return res;
    }
    bool flag=model.removeFriend(user,friendName);
    if(flag){
        FriendBlockModel blockModel;
        blockModel.removeAllBlock(user,friendName);
        LOG_INFO<<user<<"删除好友成功:"<<friendName;
        res["errno"]=0;
        res["message"]="删除好友成功";
    }else{
        LOG_ERROR<<user<<"删除好友失败:"<<friendName;
        res["errno"]=1;
        res["message"]="删除好友失败";
    }
    return res;
}