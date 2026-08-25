#include "LoginService.h"
#include "../../model/UserModel/UserModel.h"
#include "../../model/FileModel/FileModel.h"
#include "../../model/FriendRequestModel/FriendRequestModel.h"
#include "../../manager/OnlineUserManager/OnlineUserManager.h"
#include "../../manager/RedisManager/RedisManager.h"
#include "../FriendStatusService/FriendStatusService.h"
#include "../../netlib/net/TcpConnection/TcpConnection.h"
#include "../../protocol/MsgId.h"
#include "../../netlib/base/Logger/Logger.h"
#include "../../utils/SHA256/SHA256.h"
using namespace std;
LoginService::LoginService(){
}
LoginService::~LoginService(){
}
json LoginService::login(const json& js,TcpConnection* conn){

    string loginType=js.value("loginType","password");
    string username=js.value("username","");
    string password=js.value("password","");


    json response;
    UserModel model;
    bool flag=false;


    LOG_INFO<<"用户登录请求，登录类型:"<<loginType;

    if(loginType=="code"){

        string email=js.value("email","");
        string code=js.value("code","");


        if(!RedisManager::instance().connect()){

            LOG_ERROR<<"Redis连接失败";

            response["msgid"]=LOGIN_ACK;
            response["errno"]=1;
            response["message"]="redis error";

            conn->send(response.dump());
            return response;
        }

        string redisCode=RedisManager::instance().getVerifyCode(email);

        if(redisCode!=code){
            LOG_ERROR<<"验证码错误";

            response["msgid"]=LOGIN_ACK;
            response["errno"]=1;
            response["message"]="verify code error";

            conn->send(response.dump());
            return response;
        }



        username=model.queryUsernameByEmail(email);
        flag=!username.empty();
    }else{
        string passwordHash=HashSHA256::encode(password);
        flag=model.queryUser(username,passwordHash);
    }
    if(flag){
        LOG_INFO<<"用户登录成功，用户名:"<<username;
        response["msgid"]=LOGIN_ACK;
        response["errno"]=0;
        response["message"]="login success";
        response["username"]=username;
        conn->setUsername(username);
        conn->send(response.dump());
        OnlineUserManager::instance().addUser(username,conn);
        if(RedisManager::instance().connect()){
            RedisManager::instance().setOnline(username);
            LOG_INFO<<"更新用户在线状态成功，用户名:"<<username;
        }else{
            LOG_ERROR<<"Redis连接失败，无法更新在线状态，用户名:"<<username;
        }
        FriendStatusService::notifyOnline(username);
        LOG_INFO<<"通知好友上线成功，用户名:"<<username;
        FriendRequestModel requestModel;
        vector<FriendRequest> requests=requestModel.getRequests(username);
        LOG_INFO<<"读取好友申请数量:"<<requests.size();
        for(const auto& request:requests){
            json notify;
            notify["msgid"]=FRIEND_REQUEST_NOTIFY;
            notify["fromname"]=request.from;
            notify["time"]=request.time;
            notify["message"]=request.from+" 请求添加你为好友";
            conn->send(notify.dump());
        }
        LOG_INFO<<"好友申请发送完成，用户名:"<<username;
        LOG_INFO<<"开始加载离线私聊消息，用户:"<<username;
        if(RedisManager::instance().connect()){
            vector<string> messages=RedisManager::instance().getOfflineMessage(username);
            LOG_INFO<<"离线私聊消息数量:"<<messages.size();
            for(auto& msg:messages){
                json offlineMsg;
                offlineMsg["msgid"]=OFFLINE_MSG;
                offlineMsg["message"]=msg;
                conn->send(offlineMsg.dump());
            }
            RedisManager::instance().clearOfflineMessage(username);
            LOG_INFO<<"清理离线私聊消息成功，用户:"<<username;
        }else{
            LOG_ERROR<<"Redis连接失败，无法读取离线私聊消息，用户:"<<username;
        }
        LOG_INFO<<"开始加载离线群聊消息，用户:"<<username;
        if(RedisManager::instance().connect()){
            vector<string> messages=RedisManager::instance().getGroupOfflineMessage(username);
            LOG_INFO<<"离线群聊消息数量:"<<messages.size();
            for(auto& msg:messages){
                json offlineMsg;
                offlineMsg["msgid"]=GROUP_OFFLINE_NOTIFY;
                offlineMsg["message"]=msg;
                conn->send(offlineMsg.dump());
            }
            RedisManager::instance().clearGroupOfflineMessage(username);
            LOG_INFO<<"清理离线群聊消息成功，用户:"<<username;
        }else{
            LOG_ERROR<<"Redis连接失败，无法读取离线群聊消息，用户:"<<username;
        }
        LOG_INFO<<"开始加载离线文件请求，用户:"<<username;
        if(RedisManager::instance().connect()){
            vector<string> files=RedisManager::instance().getOfflineFile(username);
            LOG_INFO<<"离线文件请求数量:"<<files.size();
            for(auto& file:files){
                try{
                    json fileInfo=json::parse(file);
                    LOG_INFO<<"解析离线文件请求成功";
                    if(!fileInfo.is_object()){
                        LOG_ERROR<<"离线文件请求格式错误";
                        continue;
                    }
                    conn->send(fileInfo.dump());
                }catch(const exception& e){
                    LOG_ERROR<<"解析离线文件请求失败:"<<e.what();
                    LOG_ERROR<<"错误离线文件数据:"<<file;
                }
            }
            RedisManager::instance().clearOfflineFiles(username);
            LOG_INFO<<"清理离线文件请求成功，用户:"<<username;
        }else{
            LOG_ERROR<<"Redis连接失败，无法读取离线文件请求，用户:"<<username;
        }
        LOG_INFO<<"开始加载离线群邀请，用户:"<<username;
        if(RedisManager::instance().connect()){
            vector<string> invites=RedisManager::instance().getOfflineGroupInvite(username);
            LOG_INFO<<"离线群邀请数量:"<<invites.size();
            for(auto& invite:invites){
                try{
                    json inviteInfo=json::parse(invite);
                    if(!inviteInfo.is_object()){
                        LOG_ERROR<<"离线群邀请格式错误";
                        continue;
                    }
                    inviteInfo["msgid"]=GROUP_INVITE_NOTIFY;
                    conn->send(inviteInfo.dump());
                }catch(const exception& e){
                    LOG_ERROR<<"解析离线群邀请失败:"<<e.what();
                    LOG_ERROR<<"错误离线群邀请数据:"<<invite;
                }
            }
            RedisManager::instance().clearOfflineGroupInvite(username);
            LOG_INFO<<"清理离线群邀请成功，用户:"<<username;
        }else{
            LOG_ERROR<<"Redis连接失败，无法读取离线群邀请，用户:"<<username;
        }
        LOG_INFO<<"开始加载未完成文件，用户:"<<username;
        FileModel fileModel;
        vector<string> files=fileModel.getUnfinishedFiles(username);
        LOG_INFO<<"未完成文件数量:"<<files.size();
        for(auto& file:files){
            try{
                json fileInfo=json::parse(file);
                if(!fileInfo.is_object()){
                    LOG_ERROR<<"未完成文件数据格式错误";
                    continue;
                }
                fileInfo["msgid"]=FILE_RESUME_NOTIFY;
                conn->send(fileInfo.dump());
            }catch(const exception& e){
                LOG_ERROR<<"解析未完成文件失败:"<<e.what();
                LOG_ERROR<<"错误未完成文件数据:"<<file;
            }
        }
    }else{
        LOG_ERROR<<"用户登录失败，用户名:"<<username;
        response["msgid"]=LOGIN_ACK;
        response["errno"]=1;
        response["message"]="username or password error";
        conn->send(response.dump());
    }
    return response;
}
