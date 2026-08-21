#include "GroupManageService.h"
#include "../../model/GroupModel/GroupModel.h"
#include "../../netlib/net/TcpConnection/TcpConnection.h"
#include "../../manager/OnlineUserManager/OnlineUserManager.h"
#include <iostream>
#include "../../protocol/MsgId.h"
#include "../../netlib/base/Logger.h"
using namespace std;

json GroupManageService::kickMember(const json& js){
    json res;
    res["msgid"]=KICK_MEMBER_ACK;
    //
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
    //3. 判断被踢用户是否在群里
    if(!groupModel.isMember(groupName,username)){
        res["errno"]=1;
        res["message"]="user not in group";
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
    //6. 删除成员
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
    //
    if(!js.contains("groupname")||!js.contains("operator")||!js.contains("username")){
        Logger::instance().error("group add admin lack params");
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
        res["errno"]=1;
        res["message"]="group not exist";
        return res;
    }

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
    //5. 不能重复添加管理员
    if(model.isAdmin(groupname,username)){

        res["errno"]=1;
        res["message"]="already admin";
        return res;
    }
    //6. 添加管理员
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

    //1. 参数检查
    if(!js.contains("groupname")||!js.contains("operator")||!js.contains("username")){
        Logger::instance().error("group remove admin lack params");
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
        response["errno"]=1;
        response["message"]="group not exist";
        return response;
    }
    //3. 只有群主可以删除管理员
    if(!model.isOwner(groupname,operatorName)){
        Logger::instance().error("only owner can delete admin");
        response["errno"]=1;
        response["message"]="only owner can delete admin";
        return response;
    }
    //4. 判断目标是不是管理员
    if(!model.isAdmin(groupname,username)){
        response["errno"]=1;
        response["message"]="user is not admin";
        return response;
    }
    //5. 防止删除群主
    if(model.isOwner(groupname,username)){
        response["errno"]=1;
        response["message"]="cannot remove owner";
        return response;
    }

    //6. 删除管理员
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
    //1. 参数检查
    if(!js.contains("groupname")||!js.contains("operator")){
        Logger::instance().error("delete group lack params");
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
        response["errno"]=1;
        response["message"]="group not exist";
        return response;
    }
    //3. 只有群主可以解散群
    if(!model.isOwner(groupname,operatorName)){
        Logger::instance().info("only owner can delete group");
        response["errno"]=1;
        response["message"]="only owner can delete group";
        return response;
    }
    //4. 删除群
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
json GroupManageService::inviteGroup(const json& js){
    json res;
    res["msgid"]=INVITE_GROUP_ACK;
    //
    if(!js.contains("groupname")||!js.contains("operator")||!js.contains("username")){
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
        res["errno"]=1;
        res["message"]="group not exist";
        return res;
    }
    //2. 操作者权限
    if(!model.isOwner(groupname,operatorName)&&!model.isAdmin(groupname,operatorName)){
        res["errno"]=1;
        res["message"]="permission denied";
        return res;
    }
    //3. 被邀请的人是否已经在群
    if(model.isMember(groupname,username)){
        res["errno"]=1;
        res["message"]="already group member";
        return res;
    }
    //4. 加入群
    if(!model.addMember(groupname,username)){
        res["errno"]=1;
        res["message"]="invite failed";
        return res;
    }

    Logger::instance().info(operatorName+" invite "+username+" join group "+groupname);
    cout<<operatorName<<" invite "<<username<<" join group "<<groupname<<endl;
    //5. 通知被邀请人
    TcpConnection* userConn=OnlineUserManager::instance().getConnection(username);
    if(userConn){
        cout<<"invite notify send success"<<endl;
        Logger::instance().info("invite notify send success");
        json notify;
        notify["msgid"]=GROUP_INVITE_NOTIFY;
        notify["groupname"]=groupname;
        notify["message"]="you are invited to group "+groupname;
        userConn->send(notify.dump());
    }else{
        Logger::instance().info("invite user offline:"+username);
        cout<<"invite user offline:"<<username<<endl;
    }
    //7. 通知群成员有人加入
    auto members=model.getMembers(groupname);
    json joinNotify;
    joinNotify["msgid"]=GROUP_MEMBER_JOIN_NOTIFY;
    joinNotify["groupname"]=groupname;
    joinNotify["username"]=username;
    joinNotify["message"]=username+" joined group "+groupname;

    string data=joinNotify.dump();
    for(auto& member:members){
        //不通知自己
        if(member==username) continue;
        TcpConnection* conn=OnlineUserManager::instance().getConnection(member);
        if(conn)  conn->send(data);
    }
    res["errno"]=0;
    res["message"]="invite success";
    return res;
}