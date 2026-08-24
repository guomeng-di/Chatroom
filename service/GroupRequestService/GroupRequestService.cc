#include "GroupRequestService.h"
#include "../../model/GroupModel/GroupModel.h"
#include "../../model/GroupRequestModel/GroupRequestModel.h"
#include "../../manager/OnlineUserManager/OnlineUserManager.h"
#include "../../netlib/net/TcpConnection/TcpConnection.h"
#include "../../protocol/MsgId.h"
#include "../../netlib/base/Logger/Logger.h"
#include <unordered_set>
#include "../../model/UserModel/UserModel.h"
#include <string>
using namespace std;

//查看群申请
json GroupRequestService::getRequestList(const json& js){
    json res;
    res["msgid"]=GET_GROUP_REQUEST_ACK;
    //
    if(!js.contains("groupname")||!js.contains("operator")){
        res["errno"]=1;
        res["message"]="lack params";
        return res;
    }
    string groupname=js["groupname"];
    string operatorName=js["operator"];

    if(groupname.empty()||operatorName.empty()){
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
    //3. 判断操作者权限
    if(!model.isOwner(groupname,operatorName)&&!model.isAdmin(groupname,operatorName)){
        res["errno"]=1;
        res["message"]="permission denied";
        return res;
    }
    //4. 获取申请列表
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
    //
    if(!js.contains("groupname")||!js.contains("username")||!js.contains("operator")||!js.contains("accept")){
        LOG_ERROR<<"处理群申请失败，缺少参数";
        res["errno"]=1;
        res["message"]="lack params";
        return res;
    }
    string groupname=js["groupname"];
    string username=js["username"];
    string operatorName=js["operator"];
    int accept=js["accept"];

    //
    if(groupname.empty()||username.empty()||operatorName.empty()){
        res["errno"]=1;
        res["message"]="params cannot empty";
        return res;
    }
    //
    if(accept!=0&&accept!=1){
        res["errno"]=1;
        res["message"]="invalid accept value";
        return res;
    }
    GroupModel groupModel;
    //2. 判断群是否存在
    if(!groupModel.groupExist(groupname)){
        res["errno"]=1;
        res["message"]="group not exist";
        return res;
    }
    //检查操作者权限
    if(!groupModel.isOwner(groupname,operatorName)&&!groupModel.isAdmin(groupname,operatorName)){
        LOG_ERROR<<"处理群申请失败，操作者权限不足";
        res["errno"]=1;
        res["message"]="permission denied";
        return res;
    }
    //检查用户是否存在
    UserModel userModel;
    if(!userModel.queryUserByUsername(username)){
         LOG_ERROR<<"处理群申请失败，用户不存在:"<<username;
        res["errno"]=1;
        res["message"]="user not exist";
        return res;
    }
    //5. 判断用户是否已经在群里
    if(groupModel.isMember(groupname,username)){
        res["errno"]=1;
        res["message"]="already group member";
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
        LOG_ERROR<<"群申请不存在，群:"<<groupname<<" 用户:"<<username;
        res["errno"] = 1;
        res["message"] = "group request not exist";
        return res;
    }    //同意
    if(!groupModel.addMember(groupname,username)){
            LOG_ERROR<<"添加群成员失败";
            res["errno"]=1;
            res["message"]="add member failed";
            return res;
        }
        if(!requestModel.deleteRequest(groupname,username)){
            LOG_ERROR<<"删除群申请记录失败";
        res["errno"]=0;
        res["message"]="accept success";
        }
//8. 拒绝申请
    else{
        if(!requestModel.deleteRequest(groupname,username)){
            res["errno"]=1;
            res["message"]="reject failed";
            return res;
        }
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
        LOG_INFO<<"群申请处理结果通知发送成功，用户:"<<username;
    }else{
        LOG_INFO<<"用户不在线，无法发送群申请处理通知:"<<username;
    }
    return res;
}