#include "EventLoopThreadPool.h"
#include "../EventLoopThread/EventLoopThread.h"
#include "../EventLoop/EventLoop.h"
#include "../../base/Logger/Logger.h"

EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop,int numThreads):
baseLoop_(baseLoop),numThreads_(numThreads),next_(0){
    LOG_INFO<<"创建EventLoop线程池成功 线程数量="<<numThreads_;
}


EventLoopThreadPool::~EventLoopThreadPool(){
    LOG_INFO<<"销毁EventLoop线程池";
}

void EventLoopThreadPool::start(){
    LOG_INFO<<"启动EventLoop线程池";
    for(int i=0;i<numThreads_;i++){
        LOG_INFO<<"创建Sub Reactor线程 index="<<i;
        auto thread=std::make_unique<EventLoopThread>();
        EventLoop* loop=thread->startLoop();
        if(loop==nullptr){
            LOG_ERROR<<"Sub Reactor线程启动失败 index="<<i;
            continue;
        }
        threads_.push_back(std::move(thread));
        loops_.push_back(loop);
        LOG_INFO<<"Sub Reactor启动成功 index="<<i<<" EventLoop="<<loop;
    }
    LOG_INFO<<"EventLoop线程池启动完成 数量="<<loops_.size();
}
EventLoop* EventLoopThreadPool::getNextLoop(){
    if(loops_.empty()){ 
        LOG_WARN<<"Sub Reactor线程为空,使用主EventLoop";
        return baseLoop_;
    }
    EventLoop* loop=loops_[next_];
    LOG_INFO<<"分配Sub Reactor成功 index="<<next_<<" EventLoop="<<loop;
    next_++;
    if(next_>=(loops_.size())) next_=0;
    return loop;
}