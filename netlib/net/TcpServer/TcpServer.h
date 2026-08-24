// 客户端发起连接
// → mainLoop epoll_wait唤醒 listen_fd
// → Acceptor.handleRead() accept拿到客户端fd
// → TcpServer.newConnection()
//     → 调用线程池getNextLoop()选出subLoop
//     → new TcpConnection(fd, subLoop)
//         → 将客户端fd注册进subLoop的epoll
// → mainLoop回去继续等待下一个连接
// → 后续该客户端所有读写事件，全部由subLoop处理

// 管主Reactor
// 管accept
// 管sub Reactor池
#pragma once

#include <string>
#include <unordered_map>

class EventLoop;
class Acceptor;
class TcpConnection;
class EventLoopThreadPool;
class TcpServer{
    public:
      TcpServer(EventLoop& loop,const std::string& ip,int port);//外部EventLoop传过来的有循环,ip地址,端口
      ~TcpServer();

      void start();//socket-bind-listen-listen_fd加入EventLoop中
      void newConnection(int client_fd);//Acceptor通知我有新客户端，我创建TcpConnection
    private:
      EventLoop& loop_;
      std::string ip_;
      int port_;
      Acceptor* acceptor_;
      EventLoopThreadPool* threadPool_;
};
