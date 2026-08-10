#include "GroupService.h"
#include "../../netlib/base/Logger.h"
#include "../../protocol/MsgId.h"
#include "../../netlib/net/TcpConnection/TcpConnection.h"
#include "../../model/GroupModel/GroupModel.h"
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
    if(!js.contains("username")||!js.contains("groupname")){
        Logger::instance().error(
        "create group lack params"
    );
        response["errno"]=1,response["message"]="lack params";
        return response;
    }
    string owner=js["username"];
    string groupName=js["groupname"];
    if(owner.empty()||groupName.empty()){
        response["errno"]=1,response["message"]="empty";
        return response;
    }
    GroupModel model;
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
    if(!js.contains("groupname") ||!js.contains("username")){
        response["errno"]=1,response["message"]="lack params";
        return response;
    }
    string groupname=js["groupname"];
    string username=js["username"];
    GroupModel model;
    if(groupname.empty()||username.empty()){
    response["errno"]=1;
    response["message"]="params cannot empty";
    return response;
}
    if(!model.groupExist(groupname)){
        Logger::instance().error(
        username+" join group "+groupname+
        " failed, group not exist"
    );
        response["errno"]=1,response["message"]="group not exist";
        return response;
    }
    if(model.isMember(groupname,username)){
        Logger::instance().error(
        username+" already in group "+groupname
    );
        response["errno"]=1,response["message"]="already group member";
        return response;
    }

    GroupRequestModel requestModel;
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
        TcpConnection* owerConn=onlineUserManager.getConnection(owner);
        if(owerConn){
            owerConn->send(notify.dump());
        }

        //通知管理员
        unordered_set<string> admins=model.getAdmins(groupname);
        for(auto& admin:admins){
        TcpConnection* adminConn=onlineUserManager.getConnection(admin);
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
    if(!js.contains("groupname") ||!js.contains("username")) {
        response["errno"]=1,response["message"]="lack params";
        return response;
    }
    string groupName=js["groupname"];
    string username=js["username"];
    GroupModel model;
    bool flag=model.leaveGroup(groupName,username);
    if(flag){ 
         Logger::instance().info(
        username+" leave group "+groupName+" success"
    );
        response["errno"]=0,response["message"]="leave group success";}
    else{
        Logger::instance().error(
        username+" leave group "+groupName+" failed"
    );
        response["errno"]=1,response["message"]="leave group fail";}
    return response;
}
json GroupService::getGroupMembers(const json& js){
    json response;
    response["msgid"]=GROUP_MEMBER_ACK;
    response["members"]=json::array();
    if(!js.contains("groupname")){
        Logger::instance().error(
        "get group members lack groupname"
    );
        response["errno"]=1,response["message"]="lack groupName";
        return response;
    }
    string groupName=js["groupname"];
    GroupModel model;
    auto members=model.getMembers(groupName);
    for(auto& user:members)  response["members"].push_back(user);

    response["errno"]=0,response["message"]="get members success";
    return response;
}
json GroupService::getGroupList(const json& js){
    //cout<<"=====get group list====="<<endl;
    json response;
    response["msgid"]=GROUP_LIST_ACK;
    response["groups"]=json::array();
    if(!js.contains("username")){
        Logger::instance().error(
        "get group list lack username"
    );
        response["errno"]=1,response["message"]="lack username";
        return response;
    }
    string username=js["username"];
    // cout<<"username="
    // <<username
    // <<endl;
    GroupModel model;
    //cout<<"before get groups"<<endl;
    auto groups=model.getGroups(username);
    cout<<"after get groups"<<endl;
    for(auto& group:groups) response["groups"].push_back(group);

    response["errno"]=0, response["message"]="get groups success";
    return response;
}
