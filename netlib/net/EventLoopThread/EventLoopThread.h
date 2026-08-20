//一个线程，绑定一个 EventLoop 事件循环，一个线程跑一个 epoll
#pragma once
#include <thread>
#include <mutex>
#include <condition_variable>
class EventLoop;
class EventLoopThread{
public:
    EventLoopThread();
    ~EventLoopThread();
    //启动线程
    EventLoop* startLoop();
private:
    void threadFunc();
private:
    std::thread thread_;
    EventLoop* loop_;
    std::mutex mutex_;
    std::condition_variable cond_;
};