#include <iostream>
#include <cstdlib>
#include "../netlib/net/EventLoop/EventLoop.h"
#include "../netlib/net/TcpServer/TcpServer.h"
#include "../model/UserModel/UserModel.h"
#include "../netlib/base/Logger/Logger.h"
using namespace std;

int main(int argc, char* argv[]){
    //初始化日志
    InitLogger("ChatServer");

    string ip="0.0.0.0";
    int port=8888;
    // 支持：./server IP PORT
    if(argc==3){
        ip = argv[1];
        port = atoi(argv[2]);
        LOG_INFO<<"使用命令行参数进行配置";
    }else{
        //不传参数，就让用户输入
        // cout << "server ip: ";
        // cin >> ip;
        // cout << "server port: ";
        // cin >> port;
    }
    LOG_INFO << "服务器配置 IP=" <<ip<< " 端口=" <<port;
    EventLoop loop;
    LOG_INFO<<"创建EventLoop";
    TcpServer server(loop, ip, port);
    LOG_INFO<<"创建TcpServer";
    server.start();
    LOG_INFO << "聊天服务器启动成功 " << ip << ":" << port;
    loop.loop();
     LOG_WARN << "EventLoop停止";
    return 0;
}