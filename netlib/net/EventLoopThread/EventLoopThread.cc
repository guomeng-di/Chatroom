#include "EventLoopThread.h"
#include "../../base/Logger/Logger.h"
#include "../EventLoop/EventLoop.h"


EventLoopThread::EventLoopThread():loop_(nullptr){    
    LOG_INFO<<"创建EventLoopThread对象";
}
EventLoopThread::~EventLoopThread(){
    LOG_INFO<<"销毁EventLoopThread对象";
    if(thread_.joinable()){
        LOG_INFO<<"等待EventLoop线程退出";
        thread_.join();
    }
}


EventLoop* EventLoopThread::startLoop(){
    //创建新线程，新线程执行this->threadFunc()
    LOG_INFO<<"启动EventLoop线程";
    thread_=std::thread(&EventLoopThread::threadFunc,this);
    std::unique_lock<std::mutex> lock(mutex_);
    cond_.wait(lock,[this](){return loop_!=nullptr;});
    LOG_INFO<<"EventLoop线程启动完成 EventLoop="<<loop_;
    return loop_;
}
void EventLoopThread::threadFunc(){
    LOG_INFO<<"Sub Reactor线程开始运行";
    EventLoop loop;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        loop_=&loop;
    }
    LOG_INFO<<"EventLoop绑定线程成功 EventLoop="<<&loop;
    cond_.notify_one();
    LOG_INFO<<"进入EventLoop循环";
    //这个线程永久运行
    loop.loop();
    LOG_WARN<<"EventLoop循环退出";
}