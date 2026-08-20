//epoller中wait函数返回事件发生个数,那么,谁来调用wait函数,谁来处理这些事件呢?
//引入:EventLoop
//思考:EventLoop里应该有什么呢?
//1需要等待时间,故有一个Epoller
//2需要循环函数loop
//3需要停止标记quit_

#pragma once
#include "../Epoller/Epoller.h"
#include <unordered_map>
#include <vector>
#include <mutex>
#include <functional>
#include <thread>
class Channel;
class TcpConnection;
class TimerFd;
class EventLoop{
    public:
      EventLoop();
      ~EventLoop();

      void loop();
      void addChannel(Channel* channel);
      void updateChannel(Channel* channel);
      void removeChannel(Channel* channel);
      void setTimerCallback(std::function<void()> cb);

      
      void addConnection(int fd,TcpConnection* conn);
      void removeConnection(int fd);
      void deleteConnection(int fd);
      void queueInLoop(std::function<void()> cb);
      void checkConnectionTimeout();
      bool isInLoopThread();//判断线程(跨线程聊天..)
      void handleWakeup();
      void wakeup();
    private:
      Epoller epoller_;
      bool quit_;
      std::unordered_map<int,Channel*> channels_;
      std::function<void()> timerCallback_;
      //当前EventLoop管理的连接
      std::unordered_map<int,TcpConnection*> connections_;
      //创建EventLoop的线程
      std::thread::id threadId_;
      //任务队列
      std::vector<std::function<void()>> pendingFunctors_;
      std::mutex mutex_;
      TimerFd* timerFd_;
      int wakeupFd_;
      Channel* wakeupChannel_;
};
// pendingFunctors_是EventLoop之间通信的任务队列。
// 当其他线程需要操作某个EventLoop管理的资源时，不直接操作
// 而是把任务加入该EventLoop的队列
// 由EventLoop所属线程在自己的loop循环中执行，保证线程安全