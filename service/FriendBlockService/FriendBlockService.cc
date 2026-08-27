#include "FriendBlockService.h"
#include "../../model/FriendBlockModel/FriendBlockModel.h"
#include "../../model/FriendModel/FriendModel.h"
#include "../../protocol/MsgId.h"
#include "../../netlib/base/Logger/Logger.h"
using namespace std;
json FriendBlockService::addBlock(const json& js){
    LOG_INFO<<"添加屏蔽请求";
    json response;
    response["msgid"]=ADD_BLOCK_ACK;
    //1.参数检查
    if(!js.contains("username")||!js.contains("blockname")){
        LOG_ERROR<<"添加屏蔽缺少参数";
        response["errno"]=1;
        response["message"]="添加屏蔽缺少参数";
        return response;
    }
    string username=js["username"];
    string blockname=js["blockname"];
    //2.空检查
    if(username.empty()||blockname.empty()){
        LOG_ERROR<<"用户名或屏蔽用户名为空";
        response["errno"]=1;
        response["message"]="username/blockname不得为空";
        return response;
    }
    //3. 不能屏蔽自己
    if(username==blockname){
        LOG_ERROR<<"不可以屏蔽自己";
        response["errno"]=1;
        response["message"]="不可以屏蔽自己";
        return response;
    }
    FriendModel friendModel;
    //4. 必须是好友
    if(!friendModel.isFriend(username,blockname)){

        LOG_ERROR<<username
                 <<" 和 "
                 <<blockname
                 <<" 不是好友";


        response["errno"]=1;
        response["message"]="不是好友,不可以屏蔽";

        return response;
    }

    FriendBlockModel model1;
    //5. 判断是否已经屏蔽
    if(model1.isBlocked(blockname,username)){


        response["errno"]=1;
        response["message"]="你已经屏蔽过了";

        return response;
    }

    //6. 添加屏蔽
    if(model1.addBlock(username,blockname)){

        LOG_INFO<<username+" 屏蔽 "+blockname+"成功";
        response["errno"]=0;
        response["message"]="屏蔽成功";
    }else{
        LOG_ERROR<<username+"屏蔽"+blockname+" 失败";
        response["errno"]=1;
        response["message"]="屏蔽失败";
    }
    return response;
}
json FriendBlockService::removeBlock(const json& js){
    LOG_INFO<<"解除屏蔽请求";
    json response;
    response["msgid"]=REMOVE_BLOCK_ACK;
    //1. 参数检查
    if(!js.contains("username")||!js.contains("blockname")){
        LOG_ERROR<<"解除屏蔽缺少参数";
        response["errno"]=1;
        response["message"]="lack params";
        return response;
    }
    string username=js["username"];
    string blockname=js["blockname"];
    //2. 不能解除自己
    if(username==blockname){
        LOG_ERROR<<"不可以解除自己的屏蔽";
        response["errno"]=1;
        response["message"]="cannot remove block yourself";
        return response;
    }
    FriendBlockModel model1;
    //4. 判断是否存在屏蔽关系
    if(!model1.isBlocked(username,blockname)){
        LOG_ERROR<<username<<"未屏蔽"<<blockname;
        response["errno"]=1;
        response["message"]="not blocked";
        return response;
    }
    //5. 删除屏蔽
    if(model1.removeBlock(username,blockname)){

        LOG_INFO<<username<<"解除屏蔽"<<blockname+" 成功";
        response["errno"]=0;
        response["message"]="remove block success";
    }else{
        LOG_ERROR<<username<<" 解除屏蔽 "<<blockname+" 失败";
        response["errno"]=1;
        response["message"]="remove block failed";
    }
    return response;
}
bool FriendBlockService::isBlocked(const string& username,const string& blockname){
    FriendBlockModel model;
    return 
        model.isBlocked(username,blockname);
}