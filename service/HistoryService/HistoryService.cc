#include "HistoryService.h"
#include "../../model/PrivateMessageModel/PrivateMessageModel.h"
#include "../../model/GroupMessageModel/GroupMessageModel.h"
#include "../../protocol/MsgId.h"
#include "../../netlib/base/Logger.h"
#include "../../model/FriendModel/FriendModel.h"
using namespace std;
HistoryService::HistoryService(){}
HistoryService::~HistoryService(){}
// 私聊历史
json HistoryService::getPrivateHistory(const json& js){
    json response;
    response["msgid"] = GET_PRIVATE_HISTORY_ACK;
    if(!js.contains("user1") || !js.contains("user2")){
        response["errno"] = 1;
        response["message"] = "lack params";
        return response;
    }

    string user1 = js["user1"];
    string user2 = js["user2"];

    //检查好友关系
    FriendModel friendModel;
    if(!friendModel.isFriend(user1,user2)){
        response["errno"] = 1;
        response["message"] = "not friend";
        return response;
    }

    PrivateMessageModel model;
    vector<PrivateMessage> messages =model.getMessages(user1,user2);
    response["errno"] = 0;
    response["message"] = "success";
    response["messages"] = json::array();
    for(auto &msg : messages){
        json item;

        item["from"] = msg.from;
        item["to"] = msg.to;
        item["message"] = msg.message;
        item["time"] = msg.time;

        response["messages"].push_back(item);
    }

    Logger::instance().info("get private history success");
    return response;
}

// 群聊历史
json HistoryService::getGroupHistory(const json& js){
    json response;
    response["msgid"] = GET_GROUP_HISTORY_ACK;
    if(!js.contains("groupname")){
        response["errno"] = 1;
        response["message"] = "lack params";
        return response;
    }

    string groupname = js["groupname"];
    GroupMessageModel model;
    vector<GroupMessage> messages =model.getMessages(groupname);

    response["errno"] = 0;
    response["message"] = "success";
    response["messages"] = json::array();
    for(auto &msg : messages){
        json item;

        item["groupname"] = msg.groupname;
        item["from"] = msg.from;
        item["message"] = msg.message;
        item["time"] = msg.time;

        response["messages"].push_back(item);
    }
    Logger::instance().info("get group history success");
    return response;
}