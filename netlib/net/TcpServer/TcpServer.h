// EventLoop
//    |
//    +--- TcpServer
//                  |
//                  |
//                  +---- TcpConnection(Jack)

//                  |
//                  +---- TcpConnection(Tom)

//                  |
//                  +---- TcpConnection(Bob)
//    |
//    +--- Timer
//    |
//    +--- HeartBeat
#pragma once

#include <string>
#include <unordered_map>

class EventLoop;
class Acceptor;
class TcpConnection;

class TcpServer{
    public:
      TcpServer(EventLoop& loop,const std::string& ip,int port);//外部EventLoop传过来的有循环,ip地址,端口
      ~TcpServer();

      void start();//socket-bind-listen-listen_fd加入EventLoop中
      void newConnection(int client_fd);//Acceptor通知我有新客户端，我创建TcpConnection
      void checkConnectionTimeout();
    private:
      std::unordered_map<int,TcpConnection*> connections_;
      EventLoop& loop_;
      std::string ip_;
      int port_;
      Acceptor* acceptor_;
};
