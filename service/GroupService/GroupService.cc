#include "GroupService.h"
#include "../../netlib/base/Logger.h"
#include "../../protocol/MsgId.h"
#include "../../netlib/net/TcpConnection/TcpConnection.h"
#include "../../model/GroupModel/GroupModel.h"
#include "../../model/UserModel/UserModel.h"
#include "../../manager/OnlineUserManager/OnlineUserManager.h"
#include "../../model/GroupRequestModel/GroupRequestModel.h"
using namespace std;
GroupService::GroupService(){

}
GroupService::~GroupService(){

}
json GroupService::createGroup(const json& js){
    json response;
    response["msgid"]=CREATE_GROUP_ACK;
    //
    if(!js.contains("username")||!js.contains("groupname")){
        Logger::instance().error(
        "create group lack params"
    );
        response["errno"]=1,response["message"]="lack params";
        return response;
    }

    string owner=js["username"];
    string groupName=js["groupname"];
    //
    if(owner.empty()||groupName.empty()){
        response["errno"]=1,response["message"]="empty";
        return response;
    }
    GroupModel model;
    //2. 判断创建者是否存在
    UserModel userModel;
    if(!userModel.queryUserByUsername(owner)){
        Logger::instance().error("create group user not exist:"+owner );
        response["errno"]=1;
        response["message"]="user not exist";
        return response;
    }
    //3. 判断群是否已经存在
    if(model.groupExist(groupName)){
        Logger::instance().error("group already exist:"+groupName );
        response["errno"]=1;
        response["message"]="group already exist";
        return response;
    }
    //4. 创建群
    bool flag=model.createGroup(groupName,owner);
    if(flag){
        Logger::instance().info(
        owner+" create group "+groupName+" success"
    );
        response["errno"]=0;
        response["message"]="create group success";  
    }else{
        Logger::instance().error(
        owner+" create group "+groupName+" failed"
    );
        response["errno"]=1;
        response["message"]="create group fail";
    }
    return response;
}
json GroupService::joinGroup(const json& js){
    json response;
    response["msgid"]=JOIN_GROUP_ACK;
    //
    if(!js.contains("groupname") ||!js.contains("username")){
        response["errno"]=1,response["message"]="lack params";
        return response;
    }

    string groupname=js["groupname"];
    string username=js["username"];
    //
    if(groupname.empty()||username.empty()){
        response["errno"]=1;
        response["message"]="params cannot empty";
        return response;
    }

    GroupModel model;
    //2. 判断用户是否存在
    UserModel userModel;
    if(!userModel.queryUserByUsername(username)){
        response["errno"]=1;
        response["message"]="user not exist";
        return response;
    }



    //3. 判断群是否存在
    if(!model.groupExist(groupname)){
        Logger::instance().error(username+" join group "+groupname+" failed, group not exist");
        response["errno"]=1,response["message"]="group not exist";
        return response;
    }
    //4. 判断是否已经是成员
    if(model.isMember(groupname,username)){
        Logger::instance().error(username+" already in group "+groupname);
        response["errno"]=1,response["message"]="already group member";
        return response;
    }

    GroupRequestModel requestModel;
    //5. 判断是否已经申请
    auto requests=requestModel.getRequests(groupname);
    for(auto& request:requests){
        if(request.username==username){
            response["errno"]=1;
            response["message"]="already apply";
            return response;
        }
    }
    //6. 添加申请
    bool flag=requestModel.addRequest(groupname,username);
    if(flag){
        GroupModel model;
        //通知群主
        string owner=model.getOwner(groupname);
        json notify;
        notify["msgid"]=GROUP_REQUEST_NOTIFY;
        notify["groupname"]=groupname;
        notify["username"]=username;
        notify["message"]=username+" apply join group "+groupname;
        TcpConnection* owerConn=OnlineUserManager::instance().getConnection(owner);
        if(owerConn){
            owerConn->send(notify.dump());
        }

        //通知管理员
        unordered_set<string> admins=model.getAdmins(groupname);
        for(auto& admin:admins){
        TcpConnection* adminConn=OnlineUserManager::instance().getConnection(admin);
        if(adminConn){
            adminConn->send(notify.dump());
        }
    }
        response["errno"]=0;
        response["message"]="apply success";
    }else{
        response["errno"]=1;
        response["message"]="apply failed";
    }
    return response;
}
json GroupService::leaveGroup(const json& js){
    json response;
    response["msgid"]=LEAVE_GROUP_ACK;
    //
    if(!js.contains("groupname") ||!js.contains("username")) {
        response["errno"]=1,response["message"]="lack params";
        return response;
    }

    string groupName=js["groupname"];
    string username=js["username"];
    //
    if(groupName.empty()||username.empty()){
        response["errno"]=1;
        response["message"]="params cannot empty";
        return response;
    }
    GroupModel model;
    //2. 判断群存在
    if(!model.groupExist(groupName)){
        response["errno"]=1;
        response["message"]="group not exist";
        return response;
    }

    //判断用户是不是群成员
    if(!model.isMember(groupName, username)){
        response["errno"] = 1;
        response["message"] = "not group member";
        return response;
    }
    //4. 群主不能退出
    if(model.isOwner(groupName,username)){
        response["errno"]=1;
        response["message"]="owner cannot leave group";
        return response;
    }
    //获取群主
    string owner = model.getOwner(groupName);
    //获取管理员
    unordered_set<string> admins = model.getAdmins(groupName);
    //删除群成员
    bool flag=model.leaveGroup(groupName,username);
    if(!flag){
        Logger::instance().error(username+" leave group "+groupName +" failed");
        response["errno"] = 1;
        response["message"] = "leave group fail";
        return response;
    }
    Logger::instance().info(username + " leave group " +groupName + " success");
    //构造退群通知
    json notify;
    notify["msgid"]=GROUP_LEAVE_NOTIFY;
    notify["groupname"]=groupName;
    notify["username"]=username;
    notify["message"]=username +" left group " +groupName;
    //通知群主
    TcpConnection* ownerConn =OnlineUserManager::instance().getConnection(owner);
    if(ownerConn) ownerConn->send(notify.dump());
    //通知管理员
    for(const auto& admin:admins){
        // 防止管理员就是自己
        if(admin == username) continue;
        TcpConnection* adminConn = OnlineUserManager::instance().getConnection(admin);
        if(adminConn) adminConn->send(notify.dump()); 
    }
    //返回给主动退群的人
    response["errno"] = 0;
    response["message"] = "leave group success";
    return response;
}
json GroupService::getGroupMembers(const json& js){
    json response;
    response["msgid"]=GROUP_MEMBER_ACK;
    response["members"]=json::array();
    //
    if(!js.contains("groupname")){
    response["errno"]=1;
    response["message"]="lack groupname";
    return response;
}
//
if(!js.contains("username")){
    response["errno"]=1;
    response["message"]="lack username";
    return response;
}

    string groupName=js["groupname"];
    string username = js["username"];
    //
    if(groupName.empty()||username.empty()){
        response["errno"] = 1;
        response["message"]="params cannot empty";
        return response;
    }

    GroupModel model;
    //
    if(!model.groupExist(groupName)){
        response["errno"] = 1;
        response["message"] = "group not exist";
        return response;
    }
    //
    if(!model.isMember(groupName, username)){
        Logger::instance().error(username + " is not member of group " + groupName);
        response["errno"] = 1;
        response["message"] = "permission denied";
        return response;
    }
    //
    auto members=model.getMembers(groupName);
    for(auto& user:members)  response["members"].push_back(user);

    response["errno"]=0,response["message"]="get members success";
    return response;
}
json GroupService::getGroupList(const json& js){
    json response;
    response["msgid"]=GROUP_LIST_ACK;
    response["groups"]=json::array();
    //
    if(!js.contains("username")){
        Logger::instance().error("get group list lack username");
        response["errno"]=1,response["message"]="lack username";
        return response;
    }

    string username=js["username"];
    GroupModel model;

    //
    auto groups=model.getGroups(username);
    for(auto& group:groups) response["groups"].push_back(group);

    response["errno"]=0, response["message"]="get groups success";
    return response;
}
