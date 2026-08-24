#include "DeleteAccountService.h"
#include "../../model/FriendModel/FriendModel.h"
#include "../../model/UserModel/UserModel.h"
#include "../../utils/SHA256/SHA256.h"
#include "../../protocol/MsgId.h"
#include "../../model/GroupModel/GroupModel.h"
#include "../../model/GroupRequestModel/GroupRequestModel.h"

#include "../../model/FriendRequestModel/FriendRequestModel.h"
#include "../../netlib/base/Logger/Logger.h"

using namespace std;
json DeleteAccountService::removeAccount(const json& js){
    json response;
    //1
    string username=js["username"];
    string password=js["password"];

    LOG_INFO<<"收到注销账号请求 username="<<username;

    //2
    string passwordHash = HashSHA256::encode(password);
    //3查询密码对不
    UserModel model;
    if(!model.queryUser(username, passwordHash)){
        LOG_ERROR<<"注销账号失败,用户名或密码错误 username="<<username;

        response["msgid"]=DELETE_ACCOUNT_ACK;
        response["errno"]=1;
        response["message"]="username or password error";

        return response;
    }

    LOG_INFO<<"账号密码验证成功 username="<<username;

    //4-1删除好友申请
    FriendRequestModel requestModel;
    if(!requestModel.removeAllRequests(username)){
        LOG_ERROR<<"删除好友申请失败 username="<<username;

        response["msgid"]=DELETE_ACCOUNT_ACK;
        response["errno"]=1;
        response["message"]="delete friend request failed";
        return response;
    }

    LOG_INFO<<"删除好友申请成功 username="<<username;

    //4-2删除群申请
    GroupRequestModel requestModel_;
    if(!requestModel_.removeGroupRequest(username)){
        LOG_ERROR<<"删除群申请失败 username="<<username;

        response["msgid"]=DELETE_ACCOUNT_ACK;
        response["errno"]=1;
        response["message"]="delete group request failed";
        return response;
    }

    LOG_INFO<<"删除群申请成功 username="<<username;

    //4-3删除好友关系
    FriendModel friendModel;
    if(!friendModel.removeAllFriends(username)){
        LOG_ERROR<<"删除好友关系失败 username="<<username;

        response["msgid"]=DELETE_ACCOUNT_ACK;
        response["errno"]=1;
        response["message"]="delete failed";
        return response;
    }

    LOG_INFO<<"删除好友关系成功 username="<<username;

    //4-4查询我是群主的群,解散这些群chat_group
    GroupModel groupModel;
    if(groupModel.removeOwnerGroups(username)){
        LOG_ERROR<<"删除群主群失败 username="<<username;

        response["msgid"]=DELETE_ACCOUNT_ACK;
        response["errno"]=1;
        response["message"]="delete chat_group(owner) failed";
    }

    LOG_INFO<<"删除群主群完成 username="<<username;

    //4-5删除我是管理员身份的群group_admin
    if(!groupModel.removeAdmin_(username)){
        LOG_ERROR<<"删除管理员群关系失败 username="<<username;

        response["msgid"]=DELETE_ACCOUNT_ACK;
        response["errno"]=1;
        response["message"]="delete group_admin(admin) failed";
    }

    LOG_INFO<<"删除管理员群关系成功 username="<<username;

    //4-6删除加过的群(普通)group_member
    if(!groupModel.removeAllGroups(username)){
        LOG_ERROR<<"删除群成员关系失败 username="<<username;

        response["msgid"]=DELETE_ACCOUNT_ACK;
        response["errno"]=1;
        response["message"]="delete group member failed";
    }

    LOG_INFO<<"删除普通群成员关系成功 username="<<username;
    
    //4-7删除账号
    if(!model.deleteUser(username)){
        LOG_ERROR<<"删除用户账号失败 username="<<username;

        response["msgid"]=DELETE_ACCOUNT_ACK;
        response["errno"]=1;
        response["message"]="delete failed";
        return response;
    }

    LOG_INFO<<"账号注销成功 username="<<username;


    response["msgid"]=DELETE_ACCOUNT_ACK;
    response["errno"]=0;
    response["message"]="delete success";

    return response;
}