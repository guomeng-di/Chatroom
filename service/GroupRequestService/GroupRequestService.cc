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
        res["message"]="输入缺少参数";
        return res;
    }
    string groupname=js["groupname"];
    string operatorName=js["operator"];

    if(groupname.empty()||operatorName.empty()){
        res["errno"]=1;
        res["message"]="输入不得为空";
        return res;
    }
    GroupModel model;
    //2. 判断群是否存在
    if(!model.groupExist(groupname)){
        res["errno"]=1;
        res["message"]="群聊不存在";
        return res;
    }
    //3. 判断操作者权限
    if(!model.isOwner(groupname,operatorName)&&!model.isAdmin(groupname,operatorName)){
        res["errno"]=1;
        res["message"]="没有操作权限";
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
    res["message"]="获取申请列表成功";
    return res;
}
//处理群申请
json GroupRequestService::handleGroupRequest(const json& js){
    json res;
    res["msgid"]=HANDLE_GROUP_REQUEST_ACK;
    if(!js.contains("groupname")||!js.contains("username")||!js.contains("operator")||!js.contains("accept")){
        LOG_ERROR<<"处理群申请失败，缺少参数";
        res["errno"]=1;
        res["message"]="处理群申请失败，缺少参数";
        return res;
    }
    if(!js["groupname"].is_string()||!js["username"].is_string()||!js["operator"].is_string()){
        LOG_ERROR<<"处理群申请失败，参数类型错误";
        res["errno"]=1;
        res["message"]="处理群申请失败，参数类型错误";
        return res;
    }
    if(!js["accept"].is_number_integer()){
        LOG_ERROR<<"处理群申请失败，accept参数类型错误";
        res["errno"]=1;
        res["message"]="accept参数类型错误";
        return res;
    }
    string groupname=js["groupname"].get<string>();
    string username=js["username"].get<string>();
    string operatorName=js["operator"].get<string>();
    int accept=js["accept"].get<int>();
    if(groupname.empty()||username.empty()||operatorName.empty()){
        LOG_ERROR<<"处理群申请失败，输入不得为空";
        res["errno"]=1;
        res["message"]="输入不得为空";
        return res;
    }
    if(accept!=0&&accept!=1){
        LOG_ERROR<<"处理群申请失败，accept接受值无效:"<<accept;
        res["errno"]=1;
        res["message"]="accept接受值无效";
        return res;
    }
    GroupModel groupModel;
    if(!groupModel.groupExist(groupname)){
        LOG_ERROR<<"处理群申请失败，群不存在:"<<groupname;
        res["errno"]=1;
        res["message"]="群不存在";
        return res;
    }
    if(!groupModel.isOwner(groupname,operatorName)&&!groupModel.isAdmin(groupname,operatorName)){
        LOG_ERROR<<"处理群申请失败，操作者权限不足，操作者:"<<operatorName;
        res["errno"]=1;
        res["message"]="处理群申请失败，操作者权限不足";
        return res;
    }
    UserModel userModel;
    if(!userModel.queryUserByUsername(username)){
        LOG_ERROR<<"处理群申请失败，用户不存在:"<<username;
        res["errno"]=1;
        res["message"]="处理群申请失败，用户不存在";
        return res;
    }
    if(groupModel.isMember(groupname,username)){
        res["errno"]=1;
        res["message"]="已经是群成员";
        return res;
    }
    GroupRequestModel requestModel;
    vector<GroupRequest> requests=requestModel.getRequests(groupname);
    bool requestExists=false;
    for(const auto& request:requests){
        if(request.username==username){
            requestExists=true;
            break;
        }
    }
    if(!requestExists){
        LOG_ERROR<<"群申请不存在，群:"<<groupname<<" 用户:"<<username;
        res["errno"]=1;
        res["message"]="群申请不存在";
        return res;
    }
    if(accept==1){
        if(!groupModel.addMember(groupname,username)){
            LOG_ERROR<<"添加群成员失败，群:"<<groupname<<" 用户:"<<username;
            res["errno"]=1;
            res["message"]="添加群成员失败";
            return res;
        }
        if(!requestModel.deleteRequest(groupname,username)){
            LOG_ERROR<<"删除群申请记录失败，群:"<<groupname<<" 用户:"<<username;
            res["errno"]=1;
            res["message"]="删除群申请记录失败";
            return res;
        }
        res["errno"]=0;
        res["message"]="同意群申请成功";
    }else{
        if(!requestModel.deleteRequest(groupname,username)){
            LOG_ERROR<<"拒绝群申请失败，删除申请记录失败，群:"<<groupname<<" 用户:"<<username;
            res["errno"]=1;
            res["message"]="拒绝群申请失败";
            return res;
        }
        res["errno"]=0;
        res["message"]="拒绝群申请成功";
    }
    TcpConnection* conn=OnlineUserManager::instance().getConnection(username);
    if(conn){
        json notify;
        notify["msgid"]=GROUP_REQUEST_NOTIFY;
        notify["groupname"]=groupname;
        notify["username"]=username;
        notify["operator"]=operatorName;
        notify["accept"]=accept;
        if(accept==1){
            notify["message"]="已经成功入群";
        }else{
            notify["message"]="被拒绝入群";
        }
        conn->send(notify.dump());
        LOG_INFO<<"群申请处理结果通知发送成功，用户:"<<username;
    }else{
        LOG_INFO<<"用户不在线，无法发送群申请处理通知:"<<username;
    }
    return res;
}