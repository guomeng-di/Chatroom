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
#include <nlohmann/json.hpp>
using namespace std;
using json=nlohmann::json;
extern string username;

int main(int argc, char* argv[]){
    InitLogger("ChatClient");
    string server_ip="0.0.0.0";
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
    LOG_INFO<<"客户端配置 IP="<<server_ip<<" 端口="<<server_port;
    // 1 socket
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd < 0){
        LOG_ERROR<<"创建socket失败";
        return -1;
    }
    LOG_INFO<<"创建socket成功";
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

    if(connect(fd, (sockaddr*)&server, sizeof(server)) < 0){
        LOG_ERROR<<"连接服务器失败";
        return -1;
    }
    LOG_INFO<<"连接服务器成功";

    MainMenu::run(fd);
    LOG_WARN<<"客户端退出";
    return 0;
}