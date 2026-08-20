#include "GroupRequestService.h"
#include "../../model/GroupModel/GroupModel.h"
#include "../../model/GroupRequestModel/GroupRequestModel.h"
#include "../../manager/OnlineUserManager/OnlineUserManager.h"
#include "../../netlib/net/TcpConnection/TcpConnection.h"
#include "../../protocol/MsgId.h"
#include "../../netlib/base/Logger.h"
#include <unordered_set>
#include "../../model/UserModel/UserModel.h"
#include <string>
using namespace std;

//查看群申请
json GroupRequestService::getRequestList(const json& js){
    json res;
    res["msgid"]=GET_GROUP_REQUEST_ACK;
    if(!js.contains("groupname")||!js.contains("operator")){
        res["errno"]=1;
        res["message"]="lack params";
        return res;
    }
    string groupname=js["groupname"];
    string operatorName=js["operator"];

    GroupModel model;
    if(!model.isOwner(groupname,operatorName)&&!model.isAdmin(groupname,operatorName)){
        res["errno"]=1;
        res["message"]="permission denied";
        return res;
    }

    GroupRequestModel requestModel;
    auto requests=requestModel.getRequests(groupname);
    res["requests"]=json::array();
    for(auto& r:requests){
        json item;
        item["username"]=r.username,item["time"]=r.time;
        res["requests"].push_back(item);
    }
    res["errno"]=0;
    res["message"]="get request success";
    return res;
}
//处理群申请
json GroupRequestService::handleGroupRequest(const json& js){
    json res;
    res["msgid"]=HANDLE_GROUP_REQUEST_ACK;
    if(!js.contains("groupname")||!js.contains("username")||!js.contains("operator")||!js.contains("accept")){
        Logger::instance().error("handle group request lack params");
        res["errno"]=1;
        res["message"]="lack params";
        return res;
    }
    string groupname=js["groupname"];
    string username=js["username"];
    string operatorName=js["operator"];
    int accept=js["accept"];

    GroupModel groupModel;
    //检查操作者权限
    if(!groupModel.isOwner(groupname,operatorName)&&!groupModel.isAdmin(groupname,operatorName)){
        Logger::instance().error("handle group request permission denied");
        res["errno"]=1;
        res["message"]="permission denied";
        return res;
    }
    //检查用户是否存在
    UserModel userModel;
    if(!userModel.queryUserByUsername(username)){
         Logger::instance().error("handle group request user not exist: "+username);
        res["errno"]=1;
        res["message"]="user not exist";
        return res;
    }
    //检查这个用户是否真的申请过这个群
    GroupRequestModel requestModel;
    vector<GroupRequest> requests =requestModel.getRequests(groupname);
    bool requestExists=false;
    for(const auto& request:requests){
        if(request.username == username){
            requestExists = true;
            break;
        }
    }
    if(!requestExists){
        Logger::instance().error("group request not exist: " +groupname + " " + username);
        res["errno"] = 1;
        res["message"] = "group request not exist";
        return res;
    }
    //同意
    if(accept==1){
        //加入群
        bool addResult=groupModel.addMember(groupname,username);
        if(!addResult){
            Logger::instance().error("add group member failed");
            res["errno"]=1;
            res["message"]="add member failed";
            return res;
        }
        //加入后删除申请记录
        requestModel.deleteRequest(groupname,username);
        res["errno"]=0;
        res["message"]="accept success";
    }else{//拒绝
        requestModel.deleteRequest(groupname,username);
        res["errno"]=0;
        res["message"]="reject success";

    }
    //通知申请人
    TcpConnection* conn =OnlineUserManager::instance().getConnection(username);
    if(conn){
        json notify;
        notify["msgid"]=GROUP_REQUEST_NOTIFY;
        notify["groupname"]= groupname;
        notify["accept"]=accept;
        if(accept==1) notify["message"]="join group success";
        else  notify["message"]="join group rejected";
        conn->send(notify.dump());
    }
    return res;
}