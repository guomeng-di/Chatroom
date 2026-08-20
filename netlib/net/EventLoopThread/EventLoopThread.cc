#include "EventLoopThread.h"

#include "../EventLoop/EventLoop.h"


EventLoopThread::EventLoopThread():loop_(nullptr){}
EventLoopThread::~EventLoopThread(){
    if(thread_.joinable()) thread_.join();
}


EventLoop* EventLoopThread::startLoop(){
    //创建新线程，新线程执行this->threadFunc()
    thread_=std::thread(&EventLoopThread::threadFunc,this);
    std::unique_lock<std::mutex> lock(mutex_);
    cond_.wait(lock,[this](){return loop_!=nullptr;});
    return loop_;

}
void EventLoopThread::threadFunc(){
    EventLoop loop;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        loop_=&loop;
    }
    cond_.notify_one();
    //这个线程永久运行
    loop.loop();
}