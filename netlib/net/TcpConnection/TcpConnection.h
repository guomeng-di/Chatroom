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
      void send(const std::string& msg);//send()
      void handleClose();//close
      void setUsername(const std::string& username);//目的:conn保存:username+fd+msg
      std::string getUsername();

      void updateActiveTime();
      bool isTimeout();
    private:
      int fd_;
      EventLoop* loop_;
      Channel* channel_; 
      Buffer buffer_;
      std::string username_;
      time_t lastActiveTime_;
};