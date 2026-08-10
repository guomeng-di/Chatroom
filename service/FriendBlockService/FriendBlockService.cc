#include "FriendBlockService.h"
#include "../../model/FriendBlockModel/FriendBlockModel.h"
#include "../../protocol/MsgId.h"
#include "../../netlib/base/Logger.h"
#include "../../protocol/MsgId.h"

using namespace std;

json FriendBlockService::addBlock(const json& js){
    json response;
    response["msgid"]=ADD_BLOCK_ACK;
    if(!js.contains("username")||!js.contains("blockname")){
        response["errno"]=1;
        response["message"]="lack params";
        return response;
    }
    string username=js["username"];
    string blockname=js["blockname"];
    if(username==blockname){
        response["errno"]=1;
        response["message"]="cannot block yourself";
        return response;
    }

    FriendBlockModel model;
    if(model.addBlock(username,blockname)){
        response["errno"]=0;
        response["message"]="block success";
    }else{
        response["errno"]=1;
        response["message"]="block failed";
    }
    return response;
}
json FriendBlockService::removeBlock(const json& js){
    json response;
    response["msgid"]=REMOVE_BLOCK_ACK;
    if(!js.contains("username")||!js.contains("blockname")){
        response["errno"]=1;
        response["message"]="lack params";
        return response;
    }
    string username=js["username"];
    string blockname=js["blockname"];
    if(username==blockname){
        response["errno"]=1;
        response["message"]="cannot remove block yourself";
        return response;
    }

    FriendBlockModel model;
    if(model.removeBlock(username,blockname)){
        response["errno"]=0;
        response["message"]="remove block success";
    }else{
        response["errno"]=1;
        response["message"]="remove block failed";
    }
    return response;
}
bool FriendBlockService::isBlocked(const string& username,const string& blockname){
    FriendBlockModel model;
    return model.isBlocked(username,blockname);
}