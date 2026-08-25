#pragma once
#include <string>
#include <deque>
#include <cstddef>
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
      bool sendBinary(std::string msg);//send binary file
      void handleClose();//remove connection record
      void setUsername(const std::string& username);//save username and fd
      std::string getUsername();

      void updateActiveTime();
      bool isTimeout();
      int fd();
    private:
      int fd_;
      EventLoop* loop_;
      Channel* channel_;
      Buffer buffer_;
      std::deque<std::string> outputQueue_;
      std::deque<std::string> fileOutputQueue_;
      std::string outputFrame_;
      size_t outputOffset_=0;
      std::string username_;
      time_t lastActiveTime_;
      bool connected_;
};
