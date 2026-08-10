#include "DeleteAccountService.h"
#include "../../model/FriendModel/FriendModel.h"
#include "../../model/UserModel/UserModel.h"
#include "../../utils/SHA256/SHA256.h"
#include "../../protocol/MsgId.h"
#include "../../model/GroupModel/GroupModel.h"
#include "../../model/GroupRequestModel/GroupRequestModel.h"

#include "../../model/FriendRequestModel/FriendRequestModel.h"
using namespace std;
json DeleteAccountService::removeAccount(const json& js){
    json response;
    //1
    string username=js["username"];
    string password=js["password"];
    //2
    string passwordHash = HashSHA256::encode(password);
    //3查询密码对不
    UserModel model;
    if(!model.queryUser(username, passwordHash)){
        response["msgid"]=DELETE_ACCOUNT_ACK;
        response["errno"]=1;
        response["message"]="username or password error";

        return response;
    }
    //4-1删除好友申请
    FriendRequestModel requestModel;
    if(!requestModel.removeAllRequests(username)){
        response["msgid"]=DELETE_ACCOUNT_ACK;
        response["errno"]=1;
        response["message"]="delete friend request failed";
        return response;
    }
    //4-2删除群申请
    GroupRequestModel requestModel_;
    if(!requestModel_.removeGroupRequest(username)){
        response["msgid"]=DELETE_ACCOUNT_ACK;
        response["errno"]=1;
        response["message"]="delete group request failed";
        return response;
    }
    //4-3删除好友关系
    FriendModel friendModel;
    if(!friendModel.removeAllFriends(username)){
        response["msgid"]=DELETE_ACCOUNT_ACK;
        response["errno"]=1;
        response["message"]="delete failed";
        return response;
    }
    //4-4查询我是群主的群,解散这些群chat_group
    GroupModel groupModel;
    if(groupModel.removeOwnerGroups(username)){
        response["msgid"]=DELETE_ACCOUNT_ACK;
        response["errno"]=1;
        response["message"]="delete chat_group(owner) failed";
    }
    //4-5删除我是管理员身份的群group_admin
    if(!groupModel.removeAdmin_(username)){
        response["msgid"]=DELETE_ACCOUNT_ACK;
        response["errno"]=1;
        response["message"]="delete group_admin(admin) failed";
    }
    //4-6删除加过的群(普通)group_member
    if(!groupModel.removeAllGroups(username)){
        response["msgid"]=DELETE_ACCOUNT_ACK;
        response["errno"]=1;
        response["message"]="delete group member failed";
    }
    
    //4-7删除账号
    if(!model.deleteUser(username)){
        response["msgid"]=DELETE_ACCOUNT_ACK;
        response["errno"]=1;
        response["message"]="delete failed";
        return response;
    }
    response["msgid"]=DELETE_ACCOUNT_ACK;
    response["errno"]=0;
    response["message"]="delete success";

    return response;
}