#include "HistoryService.h"
#include "../../model/PrivateMessageModel/PrivateMessageModel.h"
#include "../../model/GroupMessageModel/GroupMessageModel.h"
#include "../../protocol/MsgId.h"
#include "../../netlib/base/Logger/Logger.h"
#include "../../model/FriendModel/FriendModel.h"
using namespace std;
HistoryService::HistoryService(){}
HistoryService::~HistoryService(){}
// 私聊历史
json HistoryService::getPrivateHistory(const json& js){
    json response;
    response["msgid"]=GET_PRIVATE_HISTORY_ACK;
    //1参数检查
    if(!js.contains("user1")||!js.contains("user2")){
     response["errno"]=1;
     response["message"]="lack params";
     return response;
    }

    string user1=js["user1"];
    string user2=js["user2"];

    long long beforeId=0;
    //is_number_integer判断是不是整数
    if(js.contains("beforeId") && js["beforeId"].is_number_integer()) beforeId=(long long)js["beforeId"];
    //2是不是好友
    FriendModel friendModel;
    if(!friendModel.isFriend(user1,user2)){
     response["errno"]=1;
     response["message"]="not friend";
     return response;
    }
    //3获取聊天记录
    PrivateMessageModel model;
    vector<PrivateMessage> messages=model.getMessages(user1,user2,beforeId);
    response["errno"]=0;
    response["message"]="success";
    response["messages"]=json::array();
    response["user1"]=user1;
    response["user2"]=user2;
    response["hasMore"]=messages.size()==50;
    response["nextBeforeId"]=messages.empty()?beforeId:messages.front().id;
    for(auto& msg:messages){
     json item;
     item["id"]=msg.id;
     item["from"]=msg.from;
     item["to"]=msg.to;
     item["message"]=msg.message;
     item["time"]=msg.time;
     response["messages"].push_back(item);
    }
    LOG_INFO<<"获取私聊历史记录成功";
    return response;
}

// 群聊历史
json HistoryService::getGroupHistory(const json& js){
 json response;
 response["msgid"]=GET_GROUP_HISTORY_ACK;
 if(!js.contains("groupname")){
  response["errno"]=1;
  response["message"]="lack params";
  return response;
 }
 string groupname=js["groupname"];
 long long beforeId=0;
 if(js.contains("beforeId") && js["beforeId"].is_number_integer()) beforeId=js["beforeId"].get<long long>();
 GroupMessageModel model;
 vector<GroupMessage> messages=model.getMessages(groupname,beforeId);
 response["errno"]=0;
 response["message"]="success";
 response["groupname"]=groupname;
 response["messages"]=json::array();
 response["hasMore"]=messages.size()==50;
 response["nextBeforeId"]=messages.empty()?beforeId:messages.front().id;
 for(auto& msg:messages){
  json item;
  item["id"]=msg.id;
  item["groupname"]=msg.groupname;
  item["from"]=msg.from;
  item["message"]=msg.message;
  item["time"]=msg.time;
  response["messages"].push_back(item);
 }
 LOG_INFO<<"获取群聊历史记录成功";
 return response;
}