#include "GroupManageService.h"
#include "../../model/GroupModel/GroupModel.h"
#include "../../netlib/net/TcpConnection/TcpConnection.h"
#include "../../manager/OnlineUserManager/OnlineUserManager.h"
#include "../../manager/RedisManager/RedisManager.h"
#include <iostream>
#include "../../protocol/MsgId.h"
#include "../../netlib/base/Logger/Logger.h"
using namespace std;

json GroupManageService::kickMember(const json& js){
    json res;
    res["msgid"]=KICK_MEMBER_ACK;
    //
    if(!js.contains("groupname")||!js.contains("operator")||!js.contains("username")){
        LOG_ERROR<<"踢出群成员失败，缺少参数";
        res["errno"]=1,res["message"]="lack params";
        return res;
    }

    string operatorName=js["operator"];
    string groupName=js["groupname"];
    string username=js["username"];

    //判断群是否存在
    GroupModel groupModel;
    if(!groupModel.groupExist(groupName)){
         LOG_ERROR<<"群不存在，操作用户:"<<username<<" 群:"<<groupName;
        res["errno"]=1,res["message"]="group not exist";
        return res;
    }

    //判断操作者是不是群主/群管理员
    bool permission=0;
    if(groupModel.isOwner(groupName,operatorName)||groupModel.isAdmin(groupName,operatorName)) permission=1;
    if(!permission){
        LOG_ERROR<<"踢出群成员失败，操作者权限不足";
        res["errno"]=1;
        res["message"]="permission denied";
        return res;
    }
    //3. 判断被踢用户是否在群里
    if(!groupModel.isMember(groupName,username)){
        LOG_ERROR<<"踢出群成员失败，用户不在群内:"<<username;
        res["errno"]=1;
        res["message"]="user not in group";
        return res;
    }
   //不能踢群主
    if(groupModel.isOwner(groupName,username)){
       LOG_ERROR<<"踢出群成员失败，不能踢出群主";
       res["errno"]=1;
       res["message"]="cannot kick owner";
       return res;
}   
   //管理员不能互踢
   if(groupModel.isAdmin(groupName,username)&&!groupModel.isOwner(groupName,operatorName)){
       LOG_ERROR<<"踢出群成员失败，管理员不能互相踢出";
       res["errno"]=1;
       res["message"]="admin cannot kick admin";
       return res;
    }
    //6. 删除成员
    if(groupModel.removeMember(groupName,username)){
      LOG_INFO<<"踢出群成员成功:"<<username;
      res["errno"]=0;
      res["message"]="kick success";
}else{
      LOG_ERROR<<"踢出群成员失败:"<<username;
      res["errno"]=1;
      res["message"]="kick failed";
}
    return res;
}
json GroupManageService::addAdmin(const json& js){
    json res;
    res["msgid"]=ADD_GROUP_ADMIN_ACK;
    //
    if(!js.contains("groupname")||!js.contains("operator")||!js.contains("username")){
        LOG_ERROR<<"添加管理员失败，缺少参数";
        res["errno"]=1;
        res["message"]="lack params";
    }
    string groupname=js["groupname"];
    string operatorName=js["operator"];
    string username=js["username"];

    //
    if(groupname.empty()||operatorName.empty()||username.empty()){

        res["errno"]=1;
        res["message"]="params cannot empty";
        return res;
    }

    GroupModel model;
    //2. 判断群是否存在
    if(!model.groupExist(groupname)){
        LOG_ERROR<<"添加管理员失败，群不存在:"<<groupname;
        res["errno"]=1;
        res["message"]="group not exist";
        return res;
    }

    //群主否
    if(!model.isOwner(groupname,operatorName)){
        LOG_ERROR<<"添加管理员失败，只有群主可以添加管理员";
        res["errno"]=1;
        res["message"]="only owner can add admin";
        return res;
    }
    //在群里否
    if(!model.isMember(groupname,username)){
        LOG_ERROR<<"添加管理员失败，用户不在群内:"<<username;
        res["errno"]=1;
        res["message"]="user not in group";
        return res;
    }
    //5. 不能重复添加管理员
    if(model.isAdmin(groupname,username)){

        res["errno"]=1;
        res["message"]="already admin";
        return res;
    }
    //6. 添加管理员
    if(model.addAdmin(groupname,username)){
        LOG_INFO<<"添加管理员成功:"<<username;
        res["errno"]=0;
        res["message"]="add admin success";
    }else{
        LOG_ERROR<<"添加管理员失败:"<<username;
        res["errno"]=1;
        res["message"]="add admin failed";
    }
    return res;
}
json GroupManageService::removeAdmin(const json& js){
    json response;
    response["msgid"]=REMOVE_GROUP_ADMIN_ACK;

    //1. 参数检查
    if(!js.contains("groupname")||!js.contains("operator")||!js.contains("username")){
        LOG_ERROR<<"删除管理员失败，缺少参数";
        response["errno"]=1;
        response["message"]="lack params";
        return response;
    }

    string groupname=js["groupname"];
    string operatorName=js["operator"];
    string username=js["username"];

    if(groupname.empty()||operatorName.empty()||username.empty()){
        response["errno"]=1;
        response["message"]="params cannot empty";
        return response;
    }

    GroupModel model;
    //2. 判断群是否存在
    if(!model.groupExist(groupname)){
        LOG_ERROR<<"删除管理员失败，群不存在:"<<groupname;
        response["errno"]=1;
        response["message"]="group not exist";
        return response;
    }
    //3. 只有群主可以删除管理员
    if(!model.isOwner(groupname,operatorName)){
        LOG_ERROR<<"删除管理员失败，只有群主可以删除管理员";
        response["errno"]=1;
        response["message"]="only owner can delete admin";
        return response;
    }
    //4. 判断目标是不是管理员
    if(!model.isAdmin(groupname,username)){
        LOG_ERROR<<"删除管理员失败，目标用户不是管理员:"<<username;
        response["errno"]=1;
        response["message"]="user is not admin";
        return response;
    }
    //5. 防止删除群主
    if(model.isOwner(groupname,username)){
        LOG_ERROR<<"删除管理员失败，不能删除群主权限";
        response["errno"]=1;
        response["message"]="cannot remove owner";
        return response;
    }

    //6. 删除管理员
    if(model.removeAdmin(groupname,username)){
        LOG_INFO<<"删除管理员成功:"<<username;
        response["errno"]=0;
        response["message"]="delete admin success";
    }else{
        LOG_ERROR<<"删除管理员失败:"<<username;
        response["errno"]=1;
        response["message"]="delete admin failed";
    }
    return response;
}
json GroupManageService::deleteGroup(const json& js){
    json response;
    response["msgid"]=DELETE_GROUP_ACK;
    //1. 参数检查
    if(!js.contains("groupname")||!js.contains("operator")){
        LOG_ERROR<<"解散群失败，缺少参数";
        response["errno"]=1;
        response["message"]="lack params";
        return response;
    }

    string groupname=js["groupname"];
    string operatorName=js["operator"];

    if(groupname.empty()||operatorName.empty()){
        response["errno"]=1;
        response["message"]="params cannot empty";
        return response;
    }

    GroupModel model;
     //2. 判断群是否存在
    if(!model.groupExist(groupname)){
        LOG_ERROR<<"解散群失败，群不存在:"<<groupname;
        response["errno"]=1;
        response["message"]="group not exist";
        return response;
    }
    //3. 只有群主可以解散群
    if(!model.isOwner(groupname,operatorName)){
        LOG_ERROR<<"解散群失败，只有群主可以解散群";
        response["errno"]=1;
        response["message"]="only owner can delete group";
        return response;
    }
    //4. 删除群
    if(model.deleteGroup(groupname)){
        LOG_INFO<<"解散群成功:"<<groupname;
        response["errno"]=0;
        response["message"]="delete group success";
    }else{
        LOG_ERROR<<"解散群失败:"<<groupname;
        response["errno"]=1;
        response["message"]="delete group failed";
    }
    return response;
}
json GroupManageService::inviteGroup(const json& js){
    json res;
    res["msgid"]=INVITE_GROUP_ACK;
    //
    if(!js.contains("groupname")||!js.contains("operator")||!js.contains("username")){
        LOG_ERROR<<"邀请用户加入群失败，缺少参数";
        res["errno"]=1;
        res["message"]="lack params";
        return res;
    }

    string groupname=js["groupname"];
    string operatorName=js["operator"];
    string username=js["username"];

    GroupModel model;
    //1. 群存在
    if(!model.groupExist(groupname)){
        LOG_ERROR<<"邀请加入群失败，群不存在:"<<groupname;
        res["errno"]=1;
        res["message"]="group not exist";
        return res;
    }
    //2. 操作者权限
    if(!model.isOwner(groupname,operatorName)&&!model.isAdmin(groupname,operatorName)){
        LOG_ERROR<<"邀请加入群失败，操作者权限不足";
        res["errno"]=1;
        res["message"]="permission denied";
        return res;
    }
    //3. 被邀请的人是否已经在群
    if(model.isMember(groupname,username)){
        LOG_ERROR<<"邀请加入群失败，用户已经在群内:"<<username;
        res["errno"]=1;
        res["message"]="already group member";
        return res;
    }
    //4. 加入群
    if(!model.addMember(groupname,username)){
        LOG_ERROR<<"邀请加入群失败:"<<username;
        res["errno"]=1;
        res["message"]="invite failed";
        return res;
    }

    LOG_INFO<<"邀请用户加入群成功，邀请人:"<<operatorName<<" 被邀请人:"<<username<<" 群:"<<groupname;
    return res;
}