#include "FriendBlockService.h"
#include "../../model/FriendBlockModel/FriendBlockModel.h"
#include "../../model/FriendModel/FriendModel.h"
#include "../../protocol/MsgId.h"
#include "../../netlib/base/Logger.h"
#include "../../protocol/MsgId.h"

using namespace std;

json FriendBlockService::addBlock(const json& js){
    cout<<"===== add block start ====="<<endl;


    json response;
    response["msgid"]=ADD_BLOCK_ACK;

    //1.参数检查
    if(!js.contains("username")||!js.contains("blockname")){
        response["errno"]=1;
        response["message"]="缺少成分";
        return response;
    }
    string username=js["username"];
    string blockname=js["blockname"];

    //2.空检查
    if(username.empty()||blockname.empty()){
        response["errno"]=1;
        response["message"]="username/blockname不得为空";
        return response;
    }

    //3. 不能屏蔽自己
    if(username==blockname){
        response["errno"]=1;
        response["message"]="不可以屏蔽自己";
        return response;
    }

    FriendModel friendModel;
    //4. 必须是好友
    if(!friendModel.isFriend(username,blockname)){
        response["errno"]=1;
        response["message"]="不是好友,不可以屏蔽";
        return response;
    }

    FriendBlockModel model;
    //5. 判断是否已经屏蔽
    if(model.isBlocked(username,blockname)){
        response["errno"]=1;
        response["message"]="你已经屏蔽过了";
        return response;
    }
    //6. 添加屏蔽
    if(model.addBlock(username,blockname)){
        response["errno"]=0;
        response["message"]="屏蔽成功";

        cout<<"===== add block success ====="<<endl;

        
    }else{
        response["errno"]=1;
        response["message"]="屏蔽失败";
    }
    return response;
}

json FriendBlockService::removeBlock(const json& js){
    json response;
    response["msgid"]=REMOVE_BLOCK_ACK;
    //1. 参数检查
    if(!js.contains("username")||!js.contains("blockname")){
        response["errno"]=1;
        response["message"]="lack params";
        return response;
    }
    string username=js["username"];
    string blockname=js["blockname"];

    //2. 不能解除自己
    if(username==blockname){
        response["errno"]=1;
        response["message"]="cannot remove block yourself";
        return response;
    }

    FriendModel friendModel;
    // //3. 判断是否好友
    // if(!friendModel.isFriend(username,blockname)){
    //     response["errno"]=1;
    //     response["message"]="not friends, cannot remove block";
    //     return response;
    // }

    FriendBlockModel model;
    //4. 判断是否存在屏蔽关系
    if(!model.isBlocked(username,blockname)){
        response["errno"]=1;
        response["message"]="not blocked";
        return response;
    }
    //5. 删除屏蔽
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