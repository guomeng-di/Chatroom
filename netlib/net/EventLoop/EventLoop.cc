#include "EventLoop.h"
#include "../Channel/Channel.h"
#include <cstdio>
#include "../../base/Logger.h"
#include <iostream>
using namespace std;

EventLoop::EventLoop(){
    quit_=0;
}
EventLoop::~EventLoop(){
}
void EventLoop::loop(){//不断等待事件发生，然后找到对应的Channel，让Channel处理事件
    while(!quit_){
        int n=epoller_.wait(1000);//最多等1000ms=1s就要醒来检查超时
        Logger::instance().info("epoll return n="+to_string(n));
        if(n<0){
            //perror("epoller_wait");
            Logger::instance().error("epoller_wait failed");
            return ;
        }
        for(int i=0;i<n;i++){
            int fd=epoller_.getEventFd(i);
            //cout<<"event fd="<<fd<<endl;
            Logger::instance().info("event fd="+to_string(fd));
            uint32_t events=epoller_.getEvents(i);
            if(channels_.find(fd)!=channels_.end()){
                Channel* channel=channels_[fd];
                channel->setRevents(events);
                channel->handleEvent();
            }
            if(timerCallback_) timerCallback_();//每秒执行一次
        }
    }
}
void EventLoop::addChannel(Channel* channel){
    channels_[channel->fd()]=channel;
    epoller_.addFd(channel->fd(),channel->events());
}
void EventLoop::updateChannel(Channel* channel){
    int fd=channel->fd();
    if(channels_.find(fd)==channels_.end()){
        // 不在map = 还没加入epoll，执行add
        channels_[fd]=channel;
        epoller_.addFd(fd,channel->events());
    }else  // 已经存在，执行modify
        epoller_.modifyFd(fd, channel->events());
}
void EventLoop::removeChannel(Channel* channel){
    int fd=channel->fd();
    epoller_.removeFd(fd);
    channels_.erase(fd);
}
void EventLoop::setTimerCallback(std::function<void()> cb){
    timerCallback_=cb;
}