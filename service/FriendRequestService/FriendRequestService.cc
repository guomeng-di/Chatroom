#include "FriendRequestService.h"
#include "../../model/FriendModel/FriendModel.h"
#include "../../model/FriendRequestModel/FriendRequestModel.h"
#include "../../model/UserModel/UserModel.h"
#include "../../manager/OnlineUserManager/OnlineUserManager.h" 
#include "../../netlib/net/TcpConnection/TcpConnection.h"
#include "../../protocol/MsgId.h"
#include "../../netlib/base/Logger/Logger.h"

using namespace std;

FriendRequestService::FriendRequestService(){
}
FriendRequestService::~FriendRequestService(){
}
//发送好友申请
json FriendRequestService::sendRequest(const json& js){
    LOG_INFO<<"收到发送好友申请请求";
    json response;
    response["msgid"]=SEND_FRIEND_REQUEST_ACK;
    if(!js.contains("fromname")||!js.contains("toname")){
        LOG_ERROR<<"发送好友申请缺少参数";
        response["errno"]=1,response["message"]="发送好友申请缺少参数";
        return response;
    } 
    string from=js["fromname"];
    string to=js["toname"];
    LOG_INFO<<from+"向"+to+"发送好友申请";
    if(from.empty()||to.empty()){
        LOG_ERROR<<"好友申请用户名为空";
        response["errno"]=1,response["message"]="好友申请用户名为空";
        return response;
    }
    if(from==to){
        LOG_ERROR<<"不能添加自己为好友";
        response["errno"]=1,response["message"]="不能添加自己为好友";
        return response;
    }
    UserModel model;
    if(!model.queryUserByUsername(to)){
        response["errno"]=1;
        response["message"]="该用户不存在";
        return response;
    }
    FriendModel friendModel;
    if(friendModel.isFriend(from,to)){
        LOG_ERROR<<from+"和"+to+"已经是好友";
        response["errno"]=1;
        response["message"]=from+"和"+to+"已经是好友";
        return response;
    }
    FriendRequestModel model1;
    bool flag=model1.addRequest(from,to);
    if(flag){
        TcpConnection* target =OnlineUserManager::instance().getConnection(to);
        if(target!=NULL){
            json notify;
            notify["msgid"]=FRIEND_REQUEST_NOTIFY;
            notify["fromname"]=from;
            notify["message"]="friend request";
            target->send(notify.dump());
            LOG_INFO<<"发送好友申请通知给用户:"+to;
        }
        response["errno"]=0;
        response["message"]="添加好友申请成功";
    }else{
        LOG_ERROR<<"添加好友申请失败";
        response["errno"]=1;
        response["message"]="添加好友申请失败";
    }
    return response;
}
//查询好友申请
json FriendRequestService::getRequestList(const json& js){
    LOG_INFO<<"收到查询好友申请列表请求";
    json response;
    response["msgid"]=GET_FRIEND_REQUEST_ACK;
    if(!js.contains("username")){
        LOG_ERROR<<"查询好友申请缺少用户名";
        response["errno"]=1;
        response["message"]="查询好友申请缺少用户名";
        return response;
    }
    string username=js["username"];
    FriendRequestModel model;
    vector<FriendRequest> requests=model.getRequests(username);
    response["requests"]=json::array();
    for(const auto& request:requests){
        json item;
        item["fromname"]=request.from;
        item["time"]=request.time;
        response["requests"].push_back(item);
    }
    LOG_INFO<<"查询好友申请列表成功:"+username;
    response["errno"]=0;
    response["message"]="查询好友申请列表成功";
    return response;
}
//处理好友申请(同意1/拒绝0)
json FriendRequestService::handleRequest(const json& js){
    LOG_INFO<<"收到处理好友申请请求";
    json res;
    res["msgid"]=HANDLE_FRIEND_REQUEST_ACK;
    if(!js.contains("fromname")||!js.contains("toname")||!js.contains("action")){
        LOG_ERROR<<"处理好友申请缺少参数";
        res["errno"]=1;
        res["message"]="处理好友申请缺少参数";
        return res;
    }
    string from=js["fromname"];
    string to=js["toname"];
    int action=js["action"];
    if(from.empty()||to.empty()){
        LOG_ERROR<<"处理好友申请用户名为空";
        res["errno"]=1;
        res["message"]="处理好友申请用户名为空";
        return res;
    }
    if(from==to){
        LOG_ERROR<<"不能处理自己的好友申请";
        res["errno"]=1;
        res["message"]="不能处理自己的好友申请";
        return res;
    }
    if(action!=0&&action!=1){
        LOG_ERROR<<"好友申请处理动作无效";
        res["errno"]=1;
        res["message"]="好友申请处理动作无效";
        return res;
    }
    FriendRequestModel requestModel;
    bool exist=false;
    auto requests=requestModel.getRequests(to);
    for(auto& request:requests){
        if(request.from==from){
            exist=true;
            break;
        }
    }
    if(!exist){
        LOG_ERROR<<"好友申请不存在";
        res["errno"]=1;
        res["message"]="好友申请不存在";
        return res;
    }
    if(action==1){
        FriendModel friendModel;
        if(friendModel.isFriend(from,to)||friendModel.isFriend(to,from)){
            LOG_ERROR<<"双方已经是好友";
            res["errno"]=1;
            res["message"]="双方已经是好友";
            return res;
        }
        if(!friendModel.addFriend(from,to)){
            LOG_ERROR<<"添加好友关系失败";
            res["errno"]=1;
            res["message"]="添加好友关系失败";
            return res;
        }
        if(!requestModel.removeRequest(from,to)){
            LOG_ERROR<<"删除好友申请失败";
        }
        TcpConnection* fromConn=OnlineUserManager::instance().getConnection(from);
        if(fromConn){
            json msg;
            msg["msgid"]=FRIEND_REQUEST_NOTIFY;
            msg["message"]=to+"接受了你的好友申请";
            fromConn->send(msg.dump());
            LOG_INFO<<"通知好友申请通过:"+from;
        }
        LOG_INFO<<from+"和"+to+"成为好友";
        res["errno"]=0;
        res["message"]=from+"和"+to+"成为好友";
    }else{
        if(requestModel.removeRequest(from,to)){
            LOG_INFO<<to+"拒绝了"+from+"的好友申请";
            res["errno"]=0;
            res["message"]=to+"拒绝了"+from+"的好友申请";
        }else{
            LOG_ERROR<<"拒绝好友申请失败";
            res["errno"]=1;
            res["message"]="拒绝好友申请失败";
        }
    }
    return res;
}