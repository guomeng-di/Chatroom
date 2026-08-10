//epoller中wait函数返回事件发生个数,那么,谁来调用wait函数,谁来处理这些事件呢?
//引入:EventLoop
//思考:EventLoop里应该有什么呢?
//1需要等待时间,故有一个Epoller
//2需要循环函数loop
//3需要停止标记quit_

#pragma once
#include "../Epoller/Epoller.h"
#include <unordered_map>
#include <functional>
class Channel;
class EventLoop{
    public:
      EventLoop();
      ~EventLoop();

      void loop();
      void addChannel(Channel* channel);
      void updateChannel(Channel* channel);
      void removeChannel(Channel* channel);
      void setTimerCallback(std::function<void()> cb);
    private:
      Epoller epoller_;
      bool quit_;
      std::unordered_map<int,Channel*> channels_;
      std::function<void()> timerCallback_;
      
};