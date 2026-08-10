#include "GroupManageService.h"
#include "../../model/GroupModel/GroupModel.h"
#include <iostream>
#include "../../protocol/MsgId.h"
#include "../../netlib/base/Logger.h"
using namespace std;

json GroupManageService::kickMember(const json& js){
    json res;
    res["msgid"]=KICK_MEMBER_ACK;
    if(!js.contains("groupname")||!js.contains("operator")||!js.contains("username")){
        Logger::instance().error("group kick member lack params");
        res["errno"]=1,res["message"]="lack params";
        return res;
    }

    string operatorName=js["operator"];
    string groupName=js["groupname"];
    string username=js["username"];

    //判断群是否存在
    GroupModel groupModel;
    if(!groupModel.groupExist(groupName)){
         Logger::instance().error(username+" send message to group "+groupName+" but group not exist");
        res["errno"]=1,res["message"]="group not exist";
        return res;
    }

    //判断操作者是不是群主/群管理员
    bool permission=0;
    if(groupModel.isOwner(groupName,operatorName)||groupModel.isAdmin(groupName,operatorName)) permission=1;
    if(!permission){
        res["errno"]=1;
        res["message"]="permission denied";
        return res;
    }
   //不能踢群主
    if(groupModel.isOwner(groupName,username)){
       res["errno"]=1;
       res["message"]="cannot kick owner";
       return res;
}   
   //管理员不能互踢
   if(groupModel.isAdmin(groupName,username)&&!groupModel.isOwner(groupName,operatorName)){
       res["errno"]=1;
       res["message"]="admin cannot kick admin";
       return res;
    }

    if(groupModel.removeMember(groupName,username)){
      res["errno"]=0;
      res["message"]="kick success";
}else{
      res["errno"]=1;
      res["message"]="kick failed";
}
    return res;
}
json GroupManageService::addAdmin(const json& js){
    json res;
    res["msgid"]=ADD_GROUP_ADMIN_ACK;
    if(!js.contains("groupname")||!js.contains("operator")||!js.contains("username")){
        Logger::instance().error("group add admin lack params");
        res["errno"]=1;
        res["message"]="lack params";
    }
    string groupname=js["groupname"];
    string operatorName=js["operator"];
    string username=js["username"];

    GroupModel model;
    //群主否
    if(!model.isOwner(groupname,operatorName)){
        Logger::instance().error("only owner can add admin");
        res["errno"]=1;
        res["message"]="only owner can add admin";
        return res;
    }
    //在群里否
    if(!model.isMember(groupname,username)){
        Logger::instance().error("user not in group");
        res["errno"]=1;
        res["message"]="user not in group";
        return res;
    }
    if(model.addAdmin(groupname,username)){
        Logger::instance().info("add admin success");
        res["errno"]=0;
        res["message"]="add admin success";
    }else{
        Logger::instance().error("add admin failed");
        res["errno"]=1;
        res["message"]="add admin failed";
    }
    return res;
}
json GroupManageService::removeAdmin(const json& js){
    json response;
    response["msgid"]=REMOVE_GROUP_ADMIN_ACK;

    string groupname=js["groupname"];
    string operatorName=js["operator"];
    string username=js["username"];

    GroupModel model;
    if(!model.isOwner(groupname,operatorName)){
        Logger::instance().error("only owner can delete admin");
        response["errno"]=1;
        response["message"]="only owner can delete admin";
        return response;
    }
    if(model.removeAdmin(groupname,username)){
        Logger::instance().info("delete admin success");
        response["errno"]=0;
        response["message"]="delete admin success";
    }else{
        Logger::instance().error("delete admin failed");
        response["errno"]=1;
        response["message"]="delete admin failed";
    }
    return response;
}
json GroupManageService::deleteGroup(const json& js){
    json response;
    response["msgid"]=DELETE_GROUP_ACK;
    string groupname=js["groupname"];
    string operatorName=js["operator"];

    GroupModel model;
    if(!model.isOwner(groupname,operatorName)){
        Logger::instance().info("only owner can delete group");
        response["errno"]=1;
        response["message"]="only owner can delete group";
        return response;
    }

    if(model.deleteGroup(groupname)){
        Logger::instance().info("delete group success");
        response["errno"]=0;
        response["message"]="delete group success";
    }else{
        Logger::instance().error("delete group failed");
        response["errno"]=1;
        response["message"]="delete group failed";
    }
    return response;
}