#include "GroupService.h"
#include "../../netlib/base/Logger/Logger.h"
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
        LOG_ERROR<<"创建群组失败，缺少参数";
        response["errno"]=1;
        response["message"]="lack params";
        return response;
    }
    string owner=js["username"];
    string groupName=js["groupname"];
    if(owner.empty()||groupName.empty()){
        LOG_ERROR<<"创建群组失败，参数为空";
        response["errno"]=1;
        response["message"]="params cannot empty";
        return response;
    }
    if(!js["members"].is_array()||js["members"].empty()){
        LOG_ERROR<<"创建群组失败，至少需要邀请一名用户";
        response["errno"]=1;
        response["message"]="create group must invite at least one user";
        return response;
    }
    UserModel userModel;
    if(!userModel.queryUserByUsername(owner)){
        LOG_ERROR<<"创建群组失败，用户不存在:"<<owner;
        response["errno"]=1;
        response["message"]="user not exist";
        return response;
    }
    GroupModel model;
    if(model.groupExist(groupName)){
        LOG_ERROR<<"创建群组失败，群组已存在:"<<groupName;
        response["errno"]=1;
        response["message"]="group already exist";
        return response;
    }
    FriendModel friendModel;
    unordered_set<string> inviteUsers;
    for(const auto& item:js["members"]){
        if(!item.is_string()){
            LOG_ERROR<<"创建群组失败，邀请用户格式错误";
            response["errno"]=1;
            response["message"]="invalid invited user";
            return response;
        }
        string member=item.get<string>();
        if(member.empty()){
            LOG_ERROR<<"创建群组失败，被邀请用户名为空";
            response["errno"]=1;
            response["message"]="invited username cannot be empty";
            return response;
        }
        if(member==owner){
            LOG_ERROR<<"创建群组失败，不能邀请自己";
            response["errno"]=1;
            response["message"]="cannot invite yourself";
            return response;
        }
        if(inviteUsers.count(member)){
            LOG_ERROR<<"创建群组失败，重复邀请用户:"<<member;
            response["errno"]=1;
            response["message"]="duplicate invited user: "+member;
            return response;
        }
        if(!userModel.queryUserByUsername(member)){
            LOG_ERROR<<"创建群组失败，被邀请用户不存在:"<<member;
            response["errno"]=1;
            response["message"]="user not exist: "+member;
            return response;
        }
        if(!friendModel.isFriend(owner,member)){
            LOG_ERROR<<"创建群组失败，双方不是好友:"<<owner<<" 和 "<<member;
            response["errno"]=1;
            response["message"]=member+" is not your friend";
            return response;
        }
        inviteUsers.insert(member);
    }
    if(!model.createGroup(groupName,owner,inviteUsers)){
        LOG_ERROR<<"创建群组失败，数据库创建记录失败:"<<groupName;
        response["errno"]=1;
        response["message"]="create group fail";
        return response;
    }
    LOG_INFO<<"创建群组成功，群主:"<<owner<<" 群名:"<<groupName;
    for(const auto& member:inviteUsers){
        json notify;
        notify["msgid"]=GROUP_INVITE_NOTIFY;
        notify["groupname"]=groupName;
        notify["operator"]=owner;
        notify["username"]=member;
        notify["message"]=owner+" invited you to group "+groupName+", you are now a group member";
        TcpConnection* memberConn=OnlineUserManager::instance().getConnection(member);
        if(memberConn){
            LOG_INFO<<"群邀请用户在线，发送通知:"<<member;
            memberConn->send(notify.dump());
        }else{
            LOG_INFO<<"群邀请用户离线，保存离线通知:"<<member;
            if(RedisManager::instance().connect()){
                if(RedisManager::instance().saveOfflineGroupInvite(member,notify)){
                    LOG_INFO<<"保存离线群邀请成功:"<<member;
                }else{
                    LOG_ERROR<<"保存离线群邀请失败:"<<member;
                }
            }else{
                LOG_ERROR<<"保存群邀请失败，Redis连接失败";
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
    if(!js.contains("groupname")||!js.contains("username")){
        LOG_ERROR<<"加入群组失败，缺少参数";
        response["errno"]=1;
        response["message"]="lack params";
        return response;
    }
    string groupname=js["groupname"];
    string username=js["username"];
    if(groupname.empty()||username.empty()){
        LOG_ERROR<<"加入群组失败，参数为空";
        response["errno"]=1;
        response["message"]="params cannot empty";
        return response;
    }
    GroupModel model;
    UserModel userModel;
    if(!userModel.queryUserByUsername(username)){
        LOG_ERROR<<"加入群组失败，用户不存在:"<<username;
        response["errno"]=1;
        response["message"]="user not exist";
        return response;
    }
    if(!model.groupExist(groupname)){
        LOG_ERROR<<"加入群组失败，群组不存在:"<<groupname;
        response["errno"]=1;
        response["message"]="group not exist";
        return response;
    }
    if(model.isMember(groupname,username)){
        LOG_ERROR<<"加入群组失败，用户已经在群中:"<<username;
        response["errno"]=1;
        response["message"]="already group member";
        return response;
    }
    GroupRequestModel requestModel;
    auto requests=requestModel.getRequests(groupname);
    for(auto& request:requests){
        if(request.username==username){
            LOG_ERROR<<"加入群组失败，已经提交过申请:"<<username;
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
        TcpConnection* ownerConn=OnlineUserManager::instance().getConnection(owner);
        if(ownerConn){
            ownerConn->send(notify.dump());
        }
        unordered_set<string> admins=model.getAdmins(groupname);
        for(auto& admin:admins){
            TcpConnection* adminConn=OnlineUserManager::instance().getConnection(admin);
            if(adminConn){
                adminConn->send(notify.dump());
            }
        }
        LOG_INFO<<"用户申请加入群组成功:"<<username<<" 群名:"<<groupname;
        response["errno"]=0;
        response["message"]="apply success";
    }else{
        LOG_ERROR<<"用户申请加入群组失败:"<<username<<" 群名:"<<groupname;
        response["errno"]=1;
        response["message"]="apply failed";
    }
    return response;
}
json GroupService::leaveGroup(const json& js){
    json response;
    response["msgid"]=LEAVE_GROUP_ACK;
    if(!js.contains("groupname")||!js.contains("username")){
        LOG_ERROR<<"退出群组失败，缺少参数";
        response["errno"]=1;
        response["message"]="lack params";
        return response;
    }
    string groupName=js["groupname"];
    string username=js["username"];
    if(groupName.empty()||username.empty()){
        LOG_ERROR<<"退出群组失败，参数为空";
        response["errno"]=1;
        response["message"]="params cannot empty";
        return response;
    }
    GroupModel model;
    if(!model.groupExist(groupName)){
        LOG_ERROR<<"退出群组失败，群组不存在:"<<groupName;
        response["errno"]=1;
        response["message"]="group not exist";
        return response;
    }
    if(!model.isMember(groupName,username)){
        LOG_ERROR<<"退出群组失败，用户不是群成员:"<<username;
        response["errno"]=1;
        response["message"]="退出群组失败，用户不是群成员";
        return response;
    }
    if(model.isOwner(groupName,username)){
        LOG_ERROR<<"退出群组失败，群主不能退出群组:"<<username;
        response["errno"]=1;
        response["message"]="owner cannot leave group";
        return response;
    }
    string owner=model.getOwner(groupName);
    unordered_set<string> admins=model.getAdmins(groupName);

    if(model.isAdmin(groupName,username)) model.deleteAdmin(groupName,username);
    bool flag=model.leaveGroup(groupName,username);//离开成功是1
    if(!flag){
        LOG_ERROR<<"退出群组失败:"<<username<<" 群名:"<<groupName;
        response["errno"]=1;
        response["message"]="退出群组失败";
        return response;
    }
    if(model.isAdmin(groupName,username)){
    if(!model.deleteAdmin(groupName,username)){
        LOG_ERROR<<"删除管理员身份失败:"<<username<<" 群名:"<<groupName;
    }
}
    LOG_INFO<<"用户退出群组成功:"<<username<<" 群名:"<<groupName;
    json notify;
    notify["msgid"]=GROUP_LEAVE_NOTIFY;
    notify["groupname"]=groupName;
    notify["username"]=username;
    notify["message"]=username+" left group "+groupName;
    TcpConnection* ownerConn=OnlineUserManager::instance().getConnection(owner);
    if(ownerConn){
        ownerConn->send(notify.dump());
    }
    for(const auto& admin:admins){
        if(admin==username){
            continue;
        }
        TcpConnection* adminConn=OnlineUserManager::instance().getConnection(admin);
        if(adminConn){
            adminConn->send(notify.dump());
        }
    }
    response["errno"]=0;
    response["message"]="leave group success";
    return response;
}
json GroupService::getGroupMembers(const json& js){
    json response;
    response["msgid"]=GROUP_MEMBER_ACK;
    response["members"]=json::array();
    if(!js.contains("groupname")){
        LOG_ERROR<<"查询群成员失败，缺少群名";
        response["errno"]=1;
        response["message"]="lack groupname";
        return response;
    }
    if(!js.contains("username")){
        LOG_ERROR<<"查询群成员失败，缺少用户名";
        response["errno"]=1;
        response["message"]="lack username";
        return response;
    }
    string groupName=js["groupname"];
    string username=js["username"];
    if(groupName.empty()||username.empty()){
        LOG_ERROR<<"查询群成员失败，参数为空";
        response["errno"]=1;
        response["message"]="params cannot empty";
        return response;
    }
    GroupModel model;
    if(!model.groupExist(groupName)){
        LOG_ERROR<<"查询群成员失败，群组不存在:"<<groupName;
        response["errno"]=1;
        response["message"]="group not exist";
        return response;
    }
    if(!model.isMember(groupName,username)){
        LOG_ERROR<<"查询群成员失败，用户不是群成员:"<<username;
        response["errno"]=1;
        response["message"]="permission denied";
        return response;
    }
    auto members=model.getMembers(groupName);
    for(auto& user:members){
        string res1;
        //在线状态
        if(OnlineUserManager::instance().isOnline(user)){
            res1+="在线  ";
        }else{
            res1+="离线  ";
        }
        if(model.isOwner(groupName,user)){
            res1+="群主   "+user;
            response["members"].push_back(res1);
    }else if(model.isAdmin(groupName,user)){
            res1+="管理员  "+user;
            response["members"].push_back(res1);
    }else{
            res1+="普通成员 "+user;
            response["members"].push_back(res1);
    }
    }
    LOG_INFO<<"查询群成员成功，群名:"<<groupName;
    response["errno"]=0;
    response["message"]="查询群成员成功";
    return response;
}
json GroupService::getGroupList(const json& js){
    json response;
    response["msgid"]=GROUP_LIST_ACK;
    response["groups"]=json::array();
    if(!js.contains("username")){
        LOG_ERROR<<"查询群列表失败，缺少用户名";
        response["errno"]=1;
        response["message"]="查询群列表失败，缺少用户名";
        return response;
    }
    string username=js["username"];
    GroupModel model;
    auto groups=model.getGroups(username);
    for(auto& group:groups){
        response["groups"].push_back(group);
    }
    LOG_INFO<<"查询群列表成功，用户:"<<username;
    response["errno"]=0;
    response["message"]="查询群列表成功";
    return response;
}