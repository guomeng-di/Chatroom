#include "ChatController.h"
#include "../../protocol/MsgId.h"
#include "../../protocol/MessageCodec/MessageCodec.h"
#include "../../netlib/base/SocketUtil/SocketUtil.h"
#include "../menu/Color.h"
#include "../Heartbeat/Heartbeat.h"
#include <iostream>
#include <unistd.h>
#include <nlohmann/json.hpp>
#include <limits>
#include <sys/socket.h>
#include <sys/select.h>
#include <cerrno>

using namespace std;
using json=nlohmann::json;

void ChatController::privateChat(int fd,const string& username){
    string friendName;
    cout<<"好友账号:";
    cin>>friendName;
    cin.ignore(numeric_limits<streamsize>::max(),'\n');

    cout<<COLOR_GREEN;
    cout<<R"(
+--------------------------------+
|                                |
|            私聊模式             |
|                                |
+--------------------------------+
)";
    cout<<"当前好友:"<<friendName<<endl;
    cout<<"输入 quit 返回"<<endl;
    cout<<COLOR_RESET;
    cout<<COLOR_GREEN;
    cout<<"我: ";
    cout<<COLOR_RESET;

    while(true){
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
            cerr<<"select failed"<<endl;
            break;
        }

        if(heartbeatFd>=0&&FD_ISSET(heartbeatFd,&readfds)){
            Heartbeat::check(fd);
        }

        if(!FD_ISSET(STDIN_FILENO,&readfds)) continue;

        string message;
        getline(cin,message);

        if(message=="quit"){
            cout<<"退出聊天"<<endl;
            break;
        }

        if(message.empty()){
            cout<<COLOR_GREEN<<"我: "<<COLOR_RESET;
            continue;
        }

        json js;
        js["msgid"]=CHAT_MSG;
        js["from"]=username;
        js["to"]=friendName;
        js["message"]=message;

        string data=MessageCodec::encode(js.dump());
        bool sendRet=SocketUtil::sendAll(fd,data);

        if(!sendRet){
            cout<<"发送失败"<<endl;
            break;
        }

        cout<<COLOR_GREEN<<"我: "<<COLOR_RESET;
    }

    cout<<COLOR_GREEN;
    cout<<"已退出与 "<<friendName<<" 的聊天"<<endl;
    cout<<COLOR_RESET;
}

void ChatController::groupChat(int fd,const string& username){
    string groupName;
    cout<<"群名称:";
    cin>>groupName;
    cin.ignore(numeric_limits<streamsize>::max(),'\n');

    cout<<COLOR_BLUE;
    cout<<R"(
+--------------------------------+
|                                |
|             群聊模式            |
|                                |
+--------------------------------+
)";
    cout<<"当前群: "<<groupName<<endl;
    cout<<"输入 quit 返回"<<endl;
    cout<<COLOR_RESET;
    cout<<COLOR_GREEN;
    cout<<"我: ";
    cout<<COLOR_RESET;

    while(true){
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
            cerr<<"select failed"<<endl;
            break;
        }

        if(heartbeatFd>=0&&FD_ISSET(heartbeatFd,&readfds)){
            Heartbeat::check(fd);
        }

        if(!FD_ISSET(STDIN_FILENO,&readfds)) continue;

        string message;
        getline(cin,message);

        if(message=="quit"){
            cout<<"退出聊天"<<endl;
            break;
        }

        if(message.empty()){
            cout<<COLOR_GREEN<<"我: "<<COLOR_RESET;
            continue;
        }

        json js;
        js["msgid"]=GROUP_CHAT_MSG;
        js["groupname"]=groupName;
        js["from"]=username;
        js["message"]=message;

        string data=MessageCodec::encode(js.dump());
        bool sendRet=SocketUtil::sendAll(fd,data);

        if(!sendRet){
            cout<<"发送失败"<<endl;
            break;
        }

        cout<<COLOR_GREEN<<"我: "<<COLOR_RESET;
    }

    cout<<COLOR_BLUE;
    cout<<"已退出群聊 "<<groupName<<endl;
    cout<<COLOR_RESET;
}