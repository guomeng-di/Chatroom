 #include "GroupService.h"
 #include "../../netlib/base/Logger.h"
 #include "../../protocol/MsgId.h"
 #include "../../netlib/net/TcpConnection/TcpConnection.h"
 #include "../../model/GroupModel/GroupModel.h"
 #include "../../model/UserModel/UserModel.h"
 #include "../../model/FriendModel/FriendModel.h"
 #include "../../manager/OnlineUserManager/OnlineUserManager.h"
 #include "../../manager/RedisManager/RedisManager.h"
 #include "../../model/GroupRequestModel/GroupRequestModel.h"
 #include "../../model/FriendModel/FriendModel.h"
 using namespace std;
 GroupService::GroupService(){
 }
 GroupService::~GroupService(){
 }
 json GroupService::createGroup(const json& js){
    json response;
    response["msgid"]=CREATE_GROUP_ACK;
    if(!js.contains("username")||!js.contains("groupname")||!js.contains("members")){
        Logger::instance().error("create group lack params");
        response["errno"]=1;
        response["message"]="lack params";
        return response;
    }
    string owner=js["username"];
    string groupName=js["groupname"];
    if(owner.empty()||groupName.empty()){
        response["errno"]=1;
        response["message"]="params cannot empty";
        return response;
    }
    if(!js["members"].is_array()||js["members"].empty()){
        response["errno"]=1;
        response["message"]="create group must invite at least one user";
        return response;
    }
    UserModel userModel;
    if(!userModel.queryUserByUsername(owner)){
        Logger::instance().error("create group user not exist:"+owner);
        response["errno"]=1;
        response["message"]="user not exist";
        return response;
    }
    GroupModel model;
    if(model.groupExist(groupName)){
        Logger::instance().error("group already exist:"+groupName);
        response["errno"]=1;
        response["message"]="group already exist";
        return response;
    }
    FriendModel friendModel;
    unordered_set<string> inviteUsers;
    for(const auto& item:js["members"]){
        if(!item.is_string()){
            response["errno"]=1;
            response["message"]="invalid invited user";
            return response;
        }
        string member=item.get<string>();
        if(member.empty()){
            response["errno"]=1;
            response["message"]="invited username cannot be empty";
            return response;
        }
        if(member==owner){
            response["errno"]=1;
            response["message"]="cannot invite yourself";
            return response;
        }
        if(inviteUsers.count(member)){
            response["errno"]=1;
            response["message"]="duplicate invited user: "+member;
            return response;
        }
        if(!userModel.queryUserByUsername(member)){
            Logger::instance().error("invited user not exist:"+member);
            response["errno"]=1;
            response["message"]="user not exist: "+member;
            return response;
        }
        if(!friendModel.isFriend(owner,member)){
            Logger::instance().error(owner+" and "+member+" are not friends");
            response["errno"]=1;
            response["message"]=member+" is not your friend";
            return response;
        }
        inviteUsers.insert(member);
    }
    if(!model.createGroup(groupName,owner,inviteUsers)){
        Logger::instance().error(owner+" create group "+groupName+" failed");
        response["errno"]=1;
        response["message"]="create group fail";
        return response;
    }
    Logger::instance().info(owner+" create group "+groupName+" success");
    for(const auto& member:inviteUsers){
    json notify;
    notify["msgid"]=GROUP_INVITE_NOTIFY;
    notify["groupname"]=groupName;
    notify["operator"]=owner;
    notify["username"]=member;
    notify["message"]=owner+" invited you to group "+groupName+", you are now a group member";

    TcpConnection* memberConn=OnlineUserManager::instance().getConnection(member);

    if(memberConn){
        cout<<"group invite user online:"<<member<<endl;
        cout<<"send group invite notify:"<<notify.dump()<<endl;
        Logger::instance().info("group invite user online:"+member);
        memberConn->send(notify.dump());
    }else{
        cout<<"group invite user offline:"<<member<<endl;
        cout<<"save offline group invite:"<<notify.dump()<<endl;
        Logger::instance().info("group invite user offline:"+member);

        if(RedisManager::instance().connect()){
            if(RedisManager::instance().saveOfflineGroupInvite(member,notify)){
                cout<<"save offline group invite success"<<endl;
                Logger::instance().info("save offline group invite success:"+member);
            }else{
                cout<<"save offline group invite failed"<<endl;
                Logger::instance().error("save offline group invite failed:"+member);
            }
        }else{
            cout<<"redis connect failed when save group invite"<<endl;
            Logger::instance().error("redis connect failed when save group invite");
        }
    }
}
    response["errno"]=0;
    response["message"]="create group success";
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
     if(groupname.empty()||username.empty()){
         response["errno"]=1;
         response["message"]="params cannot empty";
         return response;
     }
     GroupModel model;
     UserModel userModel;
     if(!userModel.queryUserByUsername(username)){
         response["errno"]=1;
         response["message"]="user not exist";
         return response;
     }
     if(!model.groupExist(groupname)){
         Logger::instance().error(username+" join group "+groupname+" failed, group not exist");
         response["errno"]=1,response["message"]="group not exist";
         return response;
     }
     if(model.isMember(groupname,username)){
         Logger::instance().error(username+" already in group "+groupname);
         response["errno"]=1,response["message"]="already group member";
         return response;
     }
     GroupRequestModel requestModel;
     auto requests=requestModel.getRequests(groupname);
     for(auto& request:requests){
         if(request.username==username){
             response["errno"]=1;
             response["message"]="already apply";
             return response;
         }
     }
     bool flag=requestModel.addRequest(groupname,username);
     if(flag){
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
     if(!js.contains("groupname") ||!js.contains("username")){
         response["errno"]=1,response["message"]="lack params";
         return response;
     }
     string groupName=js["groupname"];
     string username=js["username"];
     if(groupName.empty()||username.empty()){
         response["errno"]=1;
         response["message"]="params cannot empty";
         return response;
     }
     GroupModel model;
     if(!model.groupExist(groupName)){
         response["errno"]=1;
         response["message"]="group not exist";
         return response;
     }
     if(!model.isMember(groupName, username)){
         response["errno"] = 1;
         response["message"] = "not group member";
         return response;
     }
     if(model.isOwner(groupName,username)){
         response["errno"]=1;
         response["message"]="owner cannot leave group";
         return response;
     }
     string owner = model.getOwner(groupName);
     unordered_set<string> admins = model.getAdmins(groupName);
     bool flag=model.leaveGroup(groupName,username);
     if(!flag){
         Logger::instance().error(username+" leave group "+groupName +" failed");
         response["errno"] = 1;
         response["message"] = "leave group fail";
         return response;
     }
     Logger::instance().info(username + " leave group " +groupName + " success");
     json notify;
     notify["msgid"]=GROUP_LEAVE_NOTIFY;
     notify["groupname"]=groupName;
     notify["username"]=username;
     notify["message"]=username +" left group " +groupName;
     TcpConnection* ownerConn =OnlineUserManager::instance().getConnection(owner);
     if(ownerConn) ownerConn->send(notify.dump());
     for(const auto& admin:admins){
         if(admin == username) continue;
         TcpConnection* adminConn = OnlineUserManager::instance().getConnection(admin);
         if(adminConn) adminConn->send(notify.dump());
     }
     response["errno"] = 0;
     response["message"] = "leave group success";
     return response;
 }
 json GroupService::getGroupMembers(const json& js){
     json response;
     response["msgid"]=GROUP_MEMBER_ACK;
     response["members"]=json::array();
     if(!js.contains("groupname")){
         response["errno"]=1;
         response["message"]="lack groupname";
         return response;
     }
     if(!js.contains("username")){
         response["errno"]=1;
         response["message"]="lack username";
         return response;
     }
     string groupName=js["groupname"];
     string username = js["username"];
     if(groupName.empty()||username.empty()){
         response["errno"] = 1;
         response["message"]="params cannot empty";
         return response;
     }
     GroupModel model;
     if(!model.groupExist(groupName)){
         response["errno"] = 1;
         response["message"] = "group not exist";
         return response;
     }
     if(!model.isMember(groupName, username)){
         Logger::instance().error(username + " is not member of group " + groupName);
         response["errno"] = 1;
         response["message"] = "permission denied";
         return response;
     }
     auto members=model.getMembers(groupName);
     for(auto& user:members) response["members"].push_back(user);
     response["errno"]=0,response["message"]="get members success";
     return response;
 }
 json GroupService::getGroupList(const json& js){
     json response;
     response["msgid"]=GROUP_LIST_ACK;
     response["groups"]=json::array();
     if(!js.contains("username")){
         Logger::instance().error("get group list lack username");
         response["errno"]=1,response["message"]="lack username";
         return response;
     }
     string username=js["username"];
     GroupModel model;
     auto groups=model.getGroups(username);
     for(auto& group:groups) response["groups"].push_back(group);
     response["errno"]=0, response["message"]="get groups success";
     return response;
 }