#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <thread>
#include <sys/timerfd.h>
#include <sys/select.h>
#include <cstdint>
#include <algorithm>
#include <string>
#include <limits>
#include "../netlib/base/Logger/Logger.h"
#include "../src/config.h"
#include "ClientMessageHandler/ClientMessageHandler.h"
#include "../protocol/MessageCodec/MessageCodec.h"
#include "../protocol/MsgId.h"
#include "../netlib/net/Buffer/Buffer.h"
#include "menu/MainMenu/MainMenu.h"
#include "menu/FriendMenu/FriendMenu.h"
#include "menu/GroupMenu/GroupMenu.h"
#include "menu/FileMenu/FileMenu.h"
#include "menu/AccountMenu/AccountMenu.h"
#include "FileClient/FileClient.h"
#include "menu/Color.h"
#include "TerminalInput/TerminalInput.h"
#include <nlohmann/json.hpp>
#include <fcntl.h>
#include <cerrno>
using namespace std;
using json=nlohmann::json;
extern string username;

int main(int argc, char* argv[]){
    system("stty sane");
    InitLogger("ChatClient");
    string server_ip="10.30.0.128";
    int server_port=8888;
    // 支持：./client IP PORT
    if(argc==3){
        server_ip =argv[1];
        server_port=atoi(argv[2]);
        LOG_INFO<<"使用命令行参数进行配置";
    }else{
        // cout<<"server ip:";
        // cin>>server_ip;

        // cout << "server port: ";
        // cin >> server_port;
    }
    cout<<"hello"<<endl;
    LOG_INFO<<"客户端配置 IP="<<server_ip<<" 端口="<<server_port;
    // 1 socket
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd < 0){
        LOG_ERROR<<"创建socket失败";
        return -1;
    }
    LOG_INFO<<"创建socket成功";
    int flags=fcntl(fd,F_GETFL,0);
if(flags<0){
    LOG_ERROR<<"获取socket flags失败";
    close(fd);
    return -1;
}
if(fcntl(fd,F_SETFL,flags|O_NONBLOCK)<0){
    LOG_ERROR<<"设置socket非阻塞失败";
    close(fd);
    return -1;
}
LOG_INFO<<"socket设置为非阻塞模式";
    // 2 connect
    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = htons(server_port);
    int ret = inet_pton(AF_INET, server_ip.c_str(), &server.sin_addr);
    if(ret == 0){
      LOG_ERROR<<"IP地址格式错误";
      return -1;
    }else if(ret < 0){
      LOG_ERROR<<"inet_pton转换失败";
      return -1;
    }

    int connRet=connect(fd,(sockaddr*)&server,sizeof(server));
if(connRet<0){
    if(errno!=EINPROGRESS){
        LOG_ERROR<<"连接服务器失败:"<<strerror(errno);
        close(fd);
        return -1;
    }
    fd_set writefds;
    FD_ZERO(&writefds);
    FD_SET(fd,&writefds);
    timeval timeout{};
    timeout.tv_sec=5;
    timeout.tv_usec=0;
    int selectRet=select(fd+1,nullptr,&writefds,nullptr,&timeout);
    if(selectRet<=0){
        LOG_ERROR<<"等待服务器连接超时";
        close(fd);
        return -1;
    }
    int socketError=0;
    socklen_t errorLen=sizeof(socketError);
    if(getsockopt(fd,SOL_SOCKET,SO_ERROR,&socketError,&errorLen)<0){
        LOG_ERROR<<"获取socket连接状态失败";
        close(fd);
        return -1;
    }
    if(socketError!=0){
        LOG_ERROR<<"连接服务器失败:"<<strerror(socketError);
        close(fd);
        return -1;
    }
}
LOG_INFO<<"连接服务器成功";
cout<<"连接服务器成功"<<endl;

    MainMenu::run(fd);
    close(fd);
    LOG_WARN<<"客户端退出";
    return 0;
}