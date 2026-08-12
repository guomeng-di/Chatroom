#include "LoginService.h"
#include "../../model/UserModel/UserModel.h"
#include "../../model/FriendRequestModel/FriendRequestModel.h"
#include "../../manager/OnlineUserManager/OnlineUserManager.h" 
#include "../../manager/RedisManager/RedisManager.h" 

#include "../../manager/RedisManager/RedisManager.h" 
#include "../FriendStatusService/FriendStatusService.h"
#include "../../netlib/net/TcpConnection/TcpConnection.h"
#include "../../protocol/MsgId.h"
#include "../../netlib/base/Logger.h"
#include "../../utils/SHA256/SHA256.h"

using namespace std;
LoginService::LoginService(){

}
LoginService::~LoginService(){

}
json LoginService::login(const json& js,TcpConnection* conn){
    string username=js["username"];
    string password=js["password"];
    Logger::instance().info("login request username="+username);

    json response;
    UserModel model;
    string passwordHash =HashSHA256::encode(password);
    bool flag=model.queryUser(username,passwordHash);
    //bool flag=model.queryUser(username,password);
    if(flag){
        Logger::instance().info(username+" login success");
        response["msgid"]=LOGIN_ACK;
        response["errno"]=0;
        response["message"]="login success";
        conn->setUsername(username);

        //发送登录成功
        conn->send(response.dump());

        //FileClient::instance().setUsername(username);
        OnlineUserManager::instance().addUser(username,conn);
        //redis修改状态
        if(RedisManager::instance().connect())RedisManager::instance().setOnline(username);
        
        FriendStatusService::notifyOnline(username);
        //cout<<"add online user: "<<username <<endl;

        //发送好友申请
        FriendRequestModel requestModel;
        vector<FriendRequest> requests = requestModel.getRequests(username);
        for(const auto& request : requests){
            json notify;
            notify["msgid"] = FRIEND_REQUEST_NOTIFY;
            notify["fromname"] = request.from;
            notify["time"] = request.time;
            notify["message"] = request.from + " 请求添加你为好友";

            conn->send(notify.dump());
        }
        //登录后读取离线消息
        if(RedisManager::instance().connect()){
            vector<string> messages =RedisManager::instance().getOfflineMessage(username);
            Logger::instance().info(username+" offline private message count="+to_string(messages.size()));
            for(auto& msg:messages){
                json offlineMsg;
                offlineMsg["msgid"]=OFFLINE_MSG;
                offlineMsg["message"]=msg;
                conn->send(offlineMsg.dump());
            }
            RedisManager::instance().clearOfflineMessage(username);
        }

        //读取群聊离线消息
        if(RedisManager::instance().connect()){
            vector<string> messages =RedisManager::instance().getGroupOfflineMessage(username);
            for(auto& msg:messages){
                json offlineMsg;
                offlineMsg["msgid"]=GROUP_OFFLINE_NOTIFY;
                offlineMsg["message"]=msg;
                conn->send(offlineMsg.dump());
            }
            RedisManager::instance().clearGroupOfflineMessage(username);
        }

        //读取离线时发送文件申请
        if(RedisManager::instance().connect()){
            vector<string> files =RedisManager::instance().getOfflineFile(username);
            for(auto& file:files){
                json offlineFile;
                offlineFile["msgid"]=FILE_REQUEST_NOTIFY;
                offlineFile["message"]=file;
                conn->send(offlineFile.dump());
            }
            RedisManager::instance().clearOfflineFiles(username);
        }

    }else{

        Logger::instance().error(username+" login failed");
        response["msgid"]=LOGIN_ACK;
        response["errno"]=1;
        response["message"]="username or password error";

        conn->send(response.dump());
    }
    return response;
}