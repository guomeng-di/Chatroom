#include "MainMenu.h"
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
#include <cstdint>
#include <algorithm>
#include <string>
#include <limits>
#include <cerrno>
#include <thread>
#include "../../../src/config.h"
#include "../../../netlib/base/Logger/Logger.h"
#include "../../ClientMessageHandler/ClientMessageHandler.h"
#include "../../Heartbeat/Heartbeat.h"
#include "../../../protocol/MessageCodec/MessageCodec.h"
#include "../../../protocol/MsgId.h"
#include "../../../netlib/net/Buffer/Buffer.h"
#include "../../../netlib/base/SocketUtil/SocketUtil.h"
#include "../FriendMenu/FriendMenu.h"
#include "../GroupMenu/GroupMenu.h"
#include "../FileMenu/FileMenu.h"
#include "../AccountMenu/AccountMenu.h"
#include "../../FileClient/FileClient.h"
#include "../Color.h"
#include <nlohmann/json.hpp>
#define ACCOUNT_LOGIN_ELSEWHERE 601
using namespace std;
using json=nlohmann::json;

string username;
Buffer clientBuffer;
string currentSendFile;

bool sendAllData(int fd,const string& data){
    return SocketUtil::sendAll(fd,data);
}

bool sendVerifyCode(int fd,const string& email);

bool getNextMessage(int fd,string& msg){
    while(true){
        if(clientBuffer.hasMessage()){
            msg=clientBuffer.retrieveMessage();
            return true;
        }
        char buf[1024*1024];
        ssize_t len=recv(fd,buf,sizeof(buf),0);
        if(len>0){
            clientBuffer.append(buf,static_cast<size_t>(len));
            continue;
        }
        if(len==0){
            // Logger::instance().error("server closed connection");
            return false;
        }
        if(errno==EINTR) continue;
        // Logger::instance().error("recv failed");
        return false;
    }
}

bool waitForJsonResponse(int fd,int expectedMsgId,json& response){
    while(true){
        string msg;
        if(!getNextMessage(fd,msg)) return false;
        int msgid=MessageCodec::getMsgId(msg);
        if(msgid==FILE_DATA_MSG){
            FilePacket packet=MessageCodec::decodeBinary(msg);
            FileClient::instance().receiveFile(packet,fd);
            continue;
        }
        if(msg.size()<4){
            // Logger::instance().error("invalid message size");
            continue;
        }
        string jsonStr(msg.data()+4,msg.size()-4);
        try{
            response=json::parse(jsonStr);
        }catch(const exception& e){
            // Logger::instance().error(string("json parse failed: ")+e.what());
            continue;
        }
        if(msgid==expectedMsgId) return true;
        ClientMessageHandler::handle(response,fd);
    }
}

void recvMessage(int fd){
    char buf[1024*1024];
    int timerFd=Heartbeat::getTimerFd();
    while(true){
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(fd,&readfds);
        FD_SET(timerFd,&readfds);
        int maxfd=max(fd,timerFd);
        timeval timeout;
        timeout.tv_sec=1;
        timeout.tv_usec=0;
        int ret=select(maxfd+1,&readfds,nullptr,nullptr,&timeout);

        if(ret<0){
            if(errno==EINTR) continue;
            break;
        }
        if(FD_ISSET(timerFd,&readfds)){
            Heartbeat::check(fd);
        }
        if(FD_ISSET(fd,&readfds)){
        int len=recv(fd,buf,sizeof(buf),0);
        if(len>0){
            clientBuffer.append(buf,len);
            while(clientBuffer.hasMessage()){
                string msg=clientBuffer.retrieveMessage();
                int msgid=MessageCodec::getMsgId(msg);
                try{
                    if(msgid==FILE_DATA_MSG){
                        FilePacket packet=MessageCodec::decodeBinary(msg);
                        FileClient::instance().receiveFile(packet,fd);
                    }else{
                        if(msg.size()<4){
                            // Logger::instance().error("invalid message size");
                            continue;
                        }
                        string jsonStr(msg.data()+4,msg.size()-4);
                        json js=json::parse(jsonStr);
                        ClientMessageHandler::handle(js,fd);
                    }
                }catch(const exception& e){
                    // Logger::instance().error(string("message error: ")+e.what());
                }
            }
        }
        
        else if(len==0){
            cout<<endl<<COLOR_RED<<"server close"<<COLOR_RESET<<endl;
            break;
        }else{
        if(errno==EINTR) continue;
        // Logger::instance().error("recv failed");
        break;
    }
}
}  Heartbeat::stop();
}

void printMainMenu(){
    cout<<COLOR_CYAN;
    cout<<R"(
==============================
           ChatRoom
==============================
 1. 好友功能
 2. 群聊功能
 3. 文件功能
 4. 账号设置
 0. 退出登录
==============================
)";
    cout<<COLOR_RESET;
}
string getPassword(){
    termios oldt,newt;

    tcgetattr(STDIN_FILENO,&oldt);

    newt=oldt;
    newt.c_lflag &= ~(ECHO);

    tcsetattr(STDIN_FILENO,TCSANOW,&newt);

    string password;
    cin>>password;

    tcsetattr(STDIN_FILENO,TCSANOW,&oldt);

    cout<<endl;

    return password;
}
bool login(int fd){
    cout<<"登录:"<<endl;
    cout<<"username:"; cin>>username;
    //大写->小写
    transform(username.begin(),username.end(),username.begin(),
    [](unsigned char c){
        return tolower(c);
    });
    string password;
    cout<<"password:"; password=getPassword();
    json loginMsg;
    loginMsg["msgid"]=LOGIN_MSG;
    loginMsg["username"]=username;
    loginMsg["password"]=password;
    string data=MessageCodec::encode(loginMsg.dump());
    if(!sendAllData(fd,data)){
        cout<<"send login message fail"<<endl;
        return false;
    }
    json response;
    if(!waitForJsonResponse(fd,LOGIN_ACK,response)){
        cout<<endl<<"server close"<<endl;
        return false;
    }
    if(!response.contains("errno")){
        cout<<endl<<"invalid login response"<<endl;
        return false;
    }
    if(response["errno"]==0){
        cout<<endl<<"login success"<<endl;
        FileClient::instance().setUsername(username);
        return true;
    }
    cout<<endl<<"login fail";
    if(response.contains("message")) cout<<": "<<response["message"];
    cout<<endl;
    return false;
}

bool loginByVerifyCode(int fd){
    cout<<"登录（验证码）"<<endl;
    //cout<<"用户名:"<<endl;
    //string username;
    //cin>>username;
    cout<<"邮箱:";

    //cin.ignore(numeric_limits<streamsize>::max(),'\n');

    string email;
    cin>>email;
    if(!sendVerifyCode(fd,email)){
        cout<<"验证码发送失败"<<endl;
        return false;
    }
    cout<<"验证码:";
    string code;
    cin>>code;
    json loginMsg;
    loginMsg["msgid"]=LOGIN_MSG;
    loginMsg["loginType"]="code";
    loginMsg["email"]=email;
    loginMsg["code"]=code;
    loginMsg["username"]=username;
    string data=MessageCodec::encode(loginMsg.dump());
    if(!sendAllData(fd,data)){
        cout<<"登录消息发送失败"<<endl;
        return false;
    }
    json response;
    if(!waitForJsonResponse(fd,LOGIN_ACK,response)){
        cout<<"服务器已断开连接"<<endl;
        return false;
    }
    if(response.value("errno",1)==0){
        username=response.value("username","");
        if(username.empty()){
            username=response.value("user","");
        }
        FileClient::instance().setUsername(username);
        cout<<"登录成功"<<endl;
        return true;
    }
    cout<<"登录失败";
    if(response.contains("message")) cout<<": "<<response["message"];
    cout<<endl;
    return false;
}

bool sendVerifyCode(int fd,const string& email){
    json js;
    js["msgid"]=SEND_VERIFY_CODE_MSG;
    js["email"]=email;
    string data=MessageCodec::encode(js.dump());
    if(!sendAllData(fd,data)){
        cout<<"验证码请求发送失败"<<endl;
        return false;
    }
    json response;
    if(!waitForJsonResponse(fd,SEND_VERIFY_CODE_ACK,response)){
        cout<<"服务器已断开连接"<<endl;
        return false;
    }
    if(!response.contains("msgid")){
        cout<<"验证码响应无效：缺少消息编号"<<endl;
        return false;
    }
    int receivedMsgId=response["msgid"];
    if(receivedMsgId!=SEND_VERIFY_CODE_ACK){
        cout<<"验证码响应无效"<<endl;
        return false;
    }
    if(response.contains("errno")&&response["errno"]==0){
        cout<<"验证码发送成功"<<endl;
        return true;
    }
    cout<<"验证码发送失败";
    if(response.contains("message")) cout<<": "<<response["message"];
    cout<<endl;
    return false;
}

bool registerUser(int fd){
    cout<<"注册"<<endl;
    cout<<"username:"; cin>>username;
    string email;
    cout<<"your email:"; cin>>email;
    if(!sendVerifyCode(fd,email)){
        cout<<"验证码发送失败"<<endl;
        return false;
    }
    string code;
    cout<<"verify code:"; cin>>code;
    string password;
    cout<<"password:"; password=getPassword();
    json regMsg;
    regMsg["msgid"]=REGISTER_MSG;
    regMsg["username"]=username;
    regMsg["email"]=email;
    regMsg["code"]=code;
    regMsg["password"]=password;
    string data=MessageCodec::encode(regMsg.dump());
    if(!sendAllData(fd,data)){
        cout<<"send register message failed"<<endl;
        return false;
    }
    json response;
    if(!waitForJsonResponse(fd,REGISTER_ACK,response)){
        cout<<endl<<"server close"<<endl;
        return false;
    }
    if(response["errno"]==0){
        cout<<"register success"<<endl;
        return true;
    }
    cout<<endl<<COLOR_RED<<"register fail: "<<response["message"]<<COLOR_RESET<<endl;
    return false;
}

bool ResetPassword(int fd){
    string email,code,password;
    cout<<"your email:"; cin>>email;
    if(!sendVerifyCode(fd,email)){
        cout<<"验证码发送失败"<<endl;
        return false;
    }
    cout<<"your verify code:"; cin>>code;
    cout<<"your new password:"; cin>>password;
    json js;
    js["msgid"]=RESET_PASSWORD_MSG;
    js["email"]=email;
    js["code"]=code;
    js["password"]=password;
    string data=MessageCodec::encode(js.dump());
    if(!sendAllData(fd,data)){
        cout<<"send reset password failed"<<endl;
        return false;
    }
    json response;
    if(!waitForJsonResponse(fd,RESET_PASSWORD_ACK,response)){
        cout<<"server close"<<endl;
        return false;
    }
    if(response["errno"]==0){
        cout<<COLOR_GREEN<<response["message"]<<COLOR_RESET<<endl;
        return true;
    }
    cout<<COLOR_RED<<response["message"]<<COLOR_RESET<<endl;
    return false;
}

void MainMenu::run(int fd){
    while(true){
        cout<<COLOR_GREEN;
        cout<<R"(
+------------------------------------+
|                                    |
|             聊天室登录             |
|                                    |
+------------------------------------+
|        1. 登录（密码）             |
|        2. 登录（验证码）           |
|        3. 注册                     |
|        4. 重置密码                 |
|        0. 退出登录                 |
+------------------------------------+
)";
        cout<<COLOR_RESET;
        int choice;
        if(!(cin>>choice)){
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(),'\n');
            cout<<endl<<"输入错误，请输入数字"<<endl;
            continue;
        }
        if(choice==1){
            if(login(fd)) break;
        }else if(choice==2){
            if(loginByVerifyCode(fd)) break;
        }else if(choice==3){
            if(registerUser(fd)){
                if(login(fd)) break;
            }
        }else if(choice==4){
            ResetPassword(fd);
        }else if(choice==0){
            cout<<COLOR_RED;
            cout<<endl<<"退出客户端"<<endl;
            cout<<COLOR_RESET;
            close(fd);
            return;
        }else{
            cout<<COLOR_YELLOW;
            cout<<endl<<"无效选择，请重新输入"<<endl;
            cout<<COLOR_RESET;
        }
    }

    if(!Heartbeat::start()){
        // Logger::instance().error("heartbeat start failed");
        close(fd);
        return;
    }

    thread t(recvMessage,fd);
    t.detach();

    cout<<"heartbeat timer started, interval=5s"<<endl;
    printMainMenu();

    while(true){
        cout.flush();
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO,&readfds);

        int heartbeatFd=Heartbeat::getTimerFd();
        if(heartbeatFd>=0) FD_SET(heartbeatFd,&readfds);

        int maxfd=STDIN_FILENO;
        if(heartbeatFd>maxfd) maxfd=heartbeatFd;

        int selectRet=select(maxfd+1,&readfds,nullptr,nullptr,nullptr);

        if(selectRet<0){
            if(errno==EINTR) continue;
            // Logger::instance().error("select");
            break;
        }

        if(heartbeatFd>=0&&FD_ISSET(heartbeatFd,&readfds)){
            Heartbeat::check(fd);
        }

        if(!FD_ISSET(STDIN_FILENO,&readfds)) continue;

        int menu;
        if(!(cin>>menu)){
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(),'\n');
            cout<<"输入错误，请输入数字"<<endl;
            continue;
        }

        switch(menu){
            case 1:
                FriendMenu::run(fd,username);
                break;
            case 2:
                GroupMenu::run(fd,username);
                break;
            case 3:
                FileMenu::run(fd,username);
                break;
            case 4:
                AccountMenu::run(fd,username);
                break;
            case 0:
                cout<<COLOR_RED;
                cout<<endl<<"退出客户端"<<endl;
                cout<<COLOR_RESET;
                Heartbeat::stop();
                close(fd);
                return;
            default:
                cout<<COLOR_YELLOW;
                cout<<endl<<"无效选择，请重新输入"<<endl;
                cout<<COLOR_RESET;
                break;
        }
        printMainMenu();
    }

    Heartbeat::stop();
    close(fd);
}
