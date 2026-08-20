#pragma once
#include <functional>
class EventLoop;
class Channel;

class TimerFd{
    public:
      TimerFd(EventLoop* loop);
      ~TimerFd();
      void setCallback(std::function<void()> cb);
      
    private:
      int timerfd_;
      EventLoop* loop_;
      Channel* channel_;
      std::function<void()> callback_;
      void handleRead();
};