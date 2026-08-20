#include <iostream>
#include <cstdlib>
#include "../netlib/net/EventLoop/EventLoop.h"
#include "../netlib/net/TcpServer/TcpServer.h"
#include "../model/UserModel/UserModel.h"
using namespace std;

int main(int argc, char* argv[]){
    string ip="0.0.0.0";
    int port;
    // 支持：./server IP PORT
    if(argc==3){
        ip = argv[1];
        port = atoi(argv[2]);
    }else{
        // 不传参数，就让用户输入
        // cout << "server ip: ";
        // cin >> ip;
        cout << "server port: ";
        cin >> port;
    }
    EventLoop loop;
    TcpServer server(loop, ip, port);
    server.start();
    cout << "server start: " << ip << ":" << port << endl;
    loop.loop();
    return 0;
}