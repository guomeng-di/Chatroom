#include "FriendRequestService.h"
#include "../../model/FriendModel/FriendModel.h"
#include "../../model/FriendRequestModel/FriendRequestModel.h"
#include "../../manager/OnlineUserManager/OnlineUserManager.h" 

#include "../../netlib/net/TcpConnection/TcpConnection.h"
#include "../../protocol/MsgId.h"

#include <iostream>
using namespace std;
extern OnlineUserManager onlineUserManager;
FriendRequestService::FriendRequestService(){
}
FriendRequestService::~FriendRequestService(){
}
//发送好友申请
json FriendRequestService::sendRequest(const json& js){
    cout<<"=====send friend request====="<<endl;
    json response;
    response["msgid"]=SEND_FRIEND_REQUEST_ACK;
    if(!js.contains("fromname")||!js.contains("toname")){
        response["errno"]=1,response["message"]="lack params";
        return response;
    } 
    string from=js["fromname"];
    string to=js["toname"];
    cout<<"from="<<from<<" to="<<to<<endl;

    if(from.empty()||to.empty()){
        response["errno"]=1,response["message"]="username empty";
        return response;
    }
    if(from==to){
        response["errno"]=1,response["message"]="cannot add yourself";
        return response;
    }
    FriendModel friendModel;
    if(friendModel.isFriend(from,to)){
        response["errno"]=1;
        response["message"]="already friends";
        return response;
    }
    FriendRequestModel model;
    bool flag=model.addRequest(from,to);
    cout<<"addRequest result="<<flag<<endl;
    if(flag){
        TcpConnection* target = onlineUserManager.getConnection(to);//根据用户名to,去在线用户管理器查找to的连接
        //to在线时,直接通知to,from给你发消息
        if(target!=NULL){
            json notify;
            notify["msgid"]=FRIEND_REQUEST_NOTIFY;
            notify["fromname"]=from;
            notify["message"]="friend request";
            target->send(notify.dump());//target就是to对应的客户端连接
        }

        response["errno"]=0;
        response["message"]="send request success";
    }else{
        response["errno"]=1;
        response["message"]="send request fail";
    }
    return response;
}
      //查询好友申请
json FriendRequestService::getRequestList(const json& js){
    json response;
    response["msgid"]=GET_FRIEND_REQUEST_ACK;
    if(!js.contains("username")){
        response["errno"]=1;
        response["message"]="lack username";
        return response;
    }
    string username=js["username"];
    FriendRequestModel model;
    vector<FriendRequest> requests=model.getRequests(username);
    response["requests"] =json::array();
    for(const auto& request : requests){
    json item;
    item["fromname"] = request.from;
    item["time"] = request.time;

    response["requests"].push_back(item);
}
    response["errno"]=0;
    response["message"]="get request success";
    return response;
}   
//处理好友申请(同意1/拒绝0)
json FriendRequestService::handleRequest(const json& js){
    json res;
    res["msgid"]=HANDLE_FRIEND_REQUEST_ACK;
    if(!js.contains("fromname") ||!js.contains("toname") ||!js.contains("action")){
        res["errno"]=1;
        res["message"]="lack params";
        return res;
    }
    string from=js["fromname"];
    string to=js["toname"];
    int action=js["action"];
    FriendRequestModel requestModel;
    // 同意好友申请
    if(action==1){
        FriendModel friendModel;
        bool flag=friendModel.addFriend(from,to);
        if(flag){
            // 添加好友成功后删除申请
            requestModel.removeRequest(from,to);
            // 通知申请人
            TcpConnection* fromConn=onlineUserManager.getConnection(from);
            if(fromConn){
                json msg;
                msg["msgid"]=FRIEND_REQUEST_NOTIFY;
                msg["message"]=to+" accepted your friend request";
                fromConn->send(msg.dump());
            }
            // 通知被申请人
            TcpConnection* toConn=onlineUserManager.getConnection(to);
            if(toConn){
                json msg;
                msg["msgid"]=FRIEND_REQUEST_NOTIFY;
                msg["message"]="you are friends with "+from;
                toConn->send(msg.dump());
            }
            res["errno"]=0;
            res["message"]=
                "accept friend success";
        }else{
            //requestModel.removeRequest(from,to);
            res["errno"]=1;
            res["message"]="accept friend fail";
        }
    }else if(action==0){
        // 拒绝申请
        bool flag=requestModel.removeRequest(from,to);
        if(flag){
            res["errno"]=0;
            res["message"]="reject friend success";
        }else{
            res["errno"]=1;
            res["message"]="reject friend fail";
        }
    }else{
        res["errno"]=1;
        res["message"]="invalid action";
    }
    return res;
}