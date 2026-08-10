#include <iostream>

#include "../netlib/net/EventLoop/EventLoop.h"
#include "../netlib/net/TcpServer/TcpServer.h"

#include "../model/UserModel/UserModel.h"

using namespace std;


int main()
{

    //========================
    // 初始化测试用户
    //========================

    // UserModel::insertUser(
    //     "tom",
    //     "1"
    // );


    // UserModel::insertUser(
    //     "jack",
    //     "11"
    // );

    // UserModel::insertUser(
    //     "lily",
    //     "111"
    // );

    EventLoop loop;


    TcpServer server(
        loop,
        "0.0.0.0",
        8888
    );


    server.start();


    cout<<"server start"<<endl;


    loop.loop();


    return 0;
}