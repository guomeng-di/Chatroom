#include "EventLoopThreadPool.h"
#include "../EventLoopThread/EventLoopThread.h"
#include "../EventLoop/EventLoop.h"

EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop,int numThreads):
baseLoop_(baseLoop),numThreads_(numThreads),next_(0){}

EventLoopThreadPool::~EventLoopThreadPool(){}

void EventLoopThreadPool::start(){
    for(int i=0;i<numThreads_;i++){
        auto thread=std::make_unique<EventLoopThread>();
        EventLoop* loop=thread->startLoop();
        threads_.push_back(std::move(thread));
        loops_.push_back(loop);
    }
}
EventLoop* EventLoopThreadPool::getNextLoop(){
    if(loops_.empty()) return baseLoop_;
    EventLoop* loop=loops_[next_];
    next_++;
    if(next_>=(loops_.size())) next_=0;
    return loop;
}