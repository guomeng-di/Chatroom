#pragma once
#include <string>
#include "../Buffer/Buffer.h"
#include <ctime>
class EventLoop;
class Channel;

class TcpConnection{
    public:
      TcpConnection(EventLoop* loop,int fd);
      ~TcpConnection();

      void handleRead();//recv()
      void handleWrite();
      void send(const std::string& msg);//send()
      bool sendBinary(std::string msg);//send()二进制文件
      void handleClose();//划掉前台的记录,去除客户端fd
      void setUsername(const std::string& username);//目的:conn保存:username+fd+msg
      std::string getUsername();

      void updateActiveTime();
      bool isTimeout();
      int fd();
    private:
      int fd_;
      EventLoop* loop_;
      Channel* channel_; 
      Buffer buffer_;
      Buffer outputBuffer_;//待发送数据
      std::string username_;
      time_t lastActiveTime_;
      bool connected_;
};