#include "EventLoop.h"
#include "../Channel/Channel.h"
#include <cstdio>
#include "../../base/Logger.h"
#include <iostream>
#include <unistd.h>
#include <sys/eventfd.h>
#include <vector>
#include "../TimerFd/TimerFd.h"
#include "../TcpConnection/TcpConnection.h"
using namespace std;

EventLoop::EventLoop():quit_(0),threadId_(std::this_thread::get_id()),timerFd_(nullptr){
    timerFd_=new TimerFd(this);
    timerFd_->setCallback(
        std::bind(&EventLoop::checkConnectionTimeout,this)
    );
    wakeupFd_=eventfd(0,EFD_NONBLOCK);
    wakeupChannel_=new Channel(this,wakeupFd_);
    wakeupChannel_->setReadCallback(bind(&EventLoop::handleWakeup,this));
    wakeupChannel_->enableReading();
}
EventLoop::~EventLoop(){
    delete timerFd_;
}
void EventLoop::loop(){//不断等待事件发生，然后找到对应的Channel，让Channel处理事件
    while(!quit_){
        int n=epoller_.wait(1000);//最多等1000ms=1s就要醒来检查超时
        Logger::instance().info("epoll return n="+to_string(n));
        if(n<0){
            Logger::instance().error("epoller_wait failed");
            return ;
        }
        for(int i=0;i<n;i++){
            int fd=epoller_.getEventFd(i);
            Logger::instance().info("event fd="+to_string(fd));
            uint32_t events=epoller_.getEvents(i);
            auto it=channels_.find(fd);
            if(it!=channels_.end()){
                Channel* channel=it->second;
                channel->setRevents(events);
                channel->handleEvent();
            }
        }
            std::vector<std::function<void()>> functors;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                functors.swap(pendingFunctors_);
            }

            for(auto& func:functors) func();
    
            //checkConnectionTimeout();
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
void EventLoop::addConnection(int fd,TcpConnection* conn){
    connections_[fd]=conn;
}

void EventLoop::queueInLoop(std::function<void()> cb){
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pendingFunctors_.push_back(cb);
    }
    wakeup();
}
void EventLoop::checkConnectionTimeout(){
    cout<<"========== CHECK CONNECTION TIMEOUT =========="<<endl;
    cout<<"EventLoop this="<<this<<endl;
    cout<<"threadId="<<threadId_<<endl;
    cout<<"connections size="<<connections_.size()<<endl;

    vector<int> timeoutFds;

    for(auto& item:connections_){
        TcpConnection* conn=item.second;
        cout<<"connection fd="<<item.first<<" username="<<conn->getUsername()<<endl;

        if(conn->isTimeout()){
            cout<<"========== CONNECTION TIMEOUT =========="<<endl;
            cout<<"fd="<<item.first<<endl;
            cout<<"username="<<conn->getUsername()<<endl;
            cout<<"========================================"<<endl;
            timeoutFds.push_back(item.first);
        }
    }

    for(int fd:timeoutFds){
        auto it=connections_.find(fd);
        if(it!=connections_.end()){
            TcpConnection* conn=it->second;
            cout<<"[TIMEOUT] calling handleClose fd="<<fd<<" username="<<conn->getUsername()<<endl;
            conn->handleClose();
        }
    }

    cout<<"=============================================="<<endl;
}
void EventLoop::deleteConnection(int fd){
    auto it=connections_.find(fd);
    if(it!=connections_.end()){
        TcpConnection* conn=it->second;
        connections_.erase(it);
        delete conn;
    }
}
bool EventLoop::isInLoopThread(){
    return threadId_==std::this_thread::get_id();
}
void EventLoop::wakeup(){
    uint64_t one=1;
    write(wakeupFd_,&one,sizeof(one));
}
void EventLoop::handleWakeup(){
    uint64_t one;
    read( wakeupFd_,&one,sizeof(one));
}
// 关闭流程
// epoll_wait
//     |
//     |
// Channel收到EPOLLHUP
//     |
//     |
// TcpConnection::handleClose()
//     |
//     |
// removeChannel()
//     |
//     |
// close(fd)
//     |
//     |
// EventLoop::deleteConnection()
//     |
//     |
// delete TcpConnection
//     |
//     |
// 析构Channel
