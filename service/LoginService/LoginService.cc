#include "LoginService.h"
#include "../../model/UserModel/UserModel.h"
#include "../../model/FileModel/FileModel.h"
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
        }// 登录后读取离线私聊消息
cout << "\n========== LOGIN STEP 1 ==========" << endl;
cout << "before getOfflineMessage, username=" << username << endl;

if(RedisManager::instance().connect()){
    vector<string> messages =
        RedisManager::instance().getOfflineMessage(username);

    cout << "after getOfflineMessage" << endl;
    cout << "private offline message count="
         << messages.size() << endl;

    for(auto& msg : messages){
        json offlineMsg;
        offlineMsg["msgid"] = OFFLINE_MSG;
        offlineMsg["message"] = msg;
        conn->send(offlineMsg.dump());
    }

    RedisManager::instance().clearOfflineMessage(username);
}

cout << "========== LOGIN STEP 1 END ==========" << endl;


// ===============================
// 离线群聊
// ===============================

cout << "\n========== LOGIN STEP 2 ==========" << endl;
cout << "before getGroupOfflineMessage, username="
     << username << endl;

if(RedisManager::instance().connect()){
    vector<string> messages =
        RedisManager::instance().getGroupOfflineMessage(username);

    cout << "after getGroupOfflineMessage" << endl;
    cout << "group offline message count="
         << messages.size() << endl;

    for(auto& msg : messages){
        json offlineMsg;
        offlineMsg["msgid"] = GROUP_OFFLINE_NOTIFY;
        offlineMsg["message"] = msg;
        conn->send(offlineMsg.dump());
    }

    RedisManager::instance().clearGroupOfflineMessage(username);
}

cout << "========== LOGIN STEP 2 END ==========" << endl;


// ===============================
// 离线文件申请
// ===============================

cout << "\n========== LOGIN STEP 3 ==========" << endl;
cout << "before getOfflineFile, username="
     << username << endl;

if(RedisManager::instance().connect()){

    vector<string> files =
        RedisManager::instance().getOfflineFile(username);

    cout << "after getOfflineFile" << endl;
    cout << "offline file count="
         << files.size() << endl;

    for(auto& file : files){

        cout << "raw offline file="
             << file
             << endl;

        try{

            json fileInfo = json::parse(file);

            cout << "parsed offline file type="
                 << fileInfo.type_name()
                 << endl;

            cout << "parsed offline file="
                 << fileInfo.dump()
                 << endl;

            if(!fileInfo.is_object()){
                cout << "ERROR: offline file is NOT object!"
                     << endl;
                continue;
            }

            conn->send(fileInfo.dump());

        }catch(const exception& e){

            cout << "ERROR parsing offline file: "
                 << e.what()
                 << endl;

            cout << "bad offline file data="
                 << file
                 << endl;
        }
    }

    RedisManager::instance().clearOfflineFiles(username);
}

cout << "========== LOGIN STEP 3 END ==========" << endl;


// ===============================
// 未完成文件
// ===============================

cout << "\n========== LOGIN STEP 4 ==========" << endl;
cout << "before getUnfinishedFiles, username="
     << username
     << endl;

FileModel fileModel;

vector<string> files =
    fileModel.getUnfinishedFiles(username);

cout << "after getUnfinishedFiles" << endl;

cout << "unfinished file count="
     << files.size()
     << endl;

for(auto& file : files){

    cout << "raw unfinished file="
         << file
         << endl;

    json fileInfo = json::parse(file);

    cout << "unfinished file type="
         << fileInfo.type_name()
         << endl;

    if(!fileInfo.is_object()){
        cout << "ERROR: unfinished file is NOT object!"
             << endl;
        continue;
    }

    fileInfo["msgid"] = FILE_RESUME_NOTIFY;

    cout << "resume notify="
         << fileInfo.dump()
         << endl;

    conn->send(fileInfo.dump());
}

cout << "========== LOGIN STEP 4 END ==========" << endl;

    }else{

        Logger::instance().error(username+" login failed");
        response["msgid"]=LOGIN_ACK;
        response["errno"]=1;
        response["message"]="username or password error";

        conn->send(response.dump());
    }
    return response;
}