#include "TimerFd.h"
#include "../EventLoop/EventLoop.h"
#include "../Channel/Channel.h"
#include <sys/timerfd.h>
#include <unistd.h>
#include "../../base/Logger.h"
TimerFd::TimerFd(EventLoop* loop):timerfd_(-1),loop_(loop),channel_(nullptr){
    timerfd_=timerfd_create(CLOCK_MONOTONIC,0);
    if(timerfd_<0){
        Logger::instance().error("timerfd create failed");
        return;
    }
    itimerspec timer{};
    timer.it_value.tv_sec=1;
    timer.it_interval.tv_sec=1;
    if(timerfd_settime(timerfd_,0,&timer,nullptr)<0){
        Logger::instance().error("timerfd settime failed");
        close(timerfd_);
        timerfd_=-1;
        return;
    }
    channel_=new Channel(loop_,timerfd_);
    channel_->setReadCallback(std::bind(&TimerFd::handleRead,this));
    channel_->enableReading();
}
TimerFd::~TimerFd(){
    if(channel_){
        loop_->removeChannel(channel_);
        delete channel_;
        channel_=nullptr;
    }
    if(timerfd_!=-1){
        close(timerfd_);
        timerfd_=-1;
    }
}
void TimerFd::handleRead(){
    uint64_t exp;
    int n=read(timerfd_,&exp,sizeof(exp));
    if(n!=sizeof(exp)){
        Logger::instance().error("timerfd read failed");
        return;
    }
    Logger::instance().info("timer tick");
    if(callback_) callback_();
}
void TimerFd::setCallback(std::function<void()> cb){
    callback_=cb;
}