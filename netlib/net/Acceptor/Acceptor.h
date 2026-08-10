#pragma once
#include <functional>
#include <string>

class EventLoop;
class Channel;

class Acceptor{
    public:
      Acceptor(EventLoop& loop,const std::string& ip,int port);
      ~Acceptor();

      void start();//TcpServer.start()内实际上是调用了Acceptor.start():socket,bind,listen_fd
      void handleRead();//处理Channel传过来的回调函数
      void setNewConnectionCallback(std::function<void(int)> cb);//保存TcpServer传来的通知函数

    private:
      EventLoop& loop_;
      std::string ip_;
      int port_;
      int listen_fd_;
      Channel* channel_;//因为listen_fd还没产生,所以不能Channel channel_;
      std::function<void(int)> newConnectionCallback_;
};

//Acceptor专门负责接收新客户端连接(listen_fd-accept()-client_fd)
//需要Channel:listen_fd需要包装