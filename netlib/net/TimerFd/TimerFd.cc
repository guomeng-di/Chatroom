#include "TimerFd.h"

#include "../EventLoop/EventLoop.h"
#include "../Channel/Channel.h"

#include <sys/timerfd.h>
#include <unistd.h>

#include "../../base/Logger.h"
TimerFd::TimerFd(EventLoop* loop):loop_(loop){
    //1创建timerfd_,计时器
    timerfd_=timerfd_create(CLOCK_MONOTONIC,0);
    if(timerfd_<0){
        Logger::instance().error("timerfd create failed");
        return;
    }
    //2设置timer
    itimerspec timer{};
    //2.1第一次1秒后触发
    timer.it_value.tv_sec=1;
    //2.1之后每1秒触发
    timer.it_interval.tv_sec=1;
    timerfd_settime(timerfd_,0,&timer,nullptr);

    //创建Channel，绑定loop,timerfd。注册读回调handleRead。
    channel_=new Channel(loop_,timerfd_);
    channel_->setReadCallback(
        std::bind(&TimerFd::handleRead,this)
    );
    channel_->enableReading();
}

TimerFd::~TimerFd(){
    if(channel_){
        loop_->removeChannel(channel_);
        delete channel_;
    }
    if(timerfd_!=-1) close(timerfd_);

}

void TimerFd::handleRead(){
    uint64_t exp;
    int n=read(timerfd_,&exp,sizeof(exp));
    if(n!=sizeof(exp)){
        Logger::instance().error("timerfd read failed");
        return;
    }
    Logger::instance().info("timer tick" );
    if(callback_) callback_();
}
void TimerFd::setCallback(std::function<void()> cb){
    callback_=cb;
}