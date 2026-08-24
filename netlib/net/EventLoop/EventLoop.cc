#include "EventLoop.h"
#include "../Channel/Channel.h"
#include <cstdio>
#include "../../base/Logger/Logger.h"
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
    LOG_INFO<<"EventLoop创建成功 threadId="<<threadId_;
}
EventLoop::~EventLoop(){
    LOG_INFO<<"EventLoop析构";
    delete timerFd_;
}
void EventLoop::loop(){//不断等待事件发生，然后找到对应的Channel，让Channel处理事件
    LOG_INFO<<"EventLoop开始运行";
    while(!quit_){
        int n=epoller_.wait(1000);//最多等1000ms=1s就要醒来检查超时
        LOG_INFO<<"epoll等待返回事件数量="<<n;
        if(n<0){
            LOG_ERROR<<"epoll_wait执行失败";
            return ;
        }
        for(int i=0;i<n;i++){
            int fd=epoller_.getEventFd(i);
            LOG_INFO<<"处理事件 fd="<<fd;
            uint32_t events=epoller_.getEvents(i);
            auto it=channels_.find(fd);
            if(it!=channels_.end()){
                Channel* channel=it->second;
                channel->setRevents(events);
                channel->handleEvent();//处理
            }
        }
//Channel 的 IO 回调必须在 EventLoop 所属 IO 线程执行
            vector<function<void()>> functors;
            {
                lock_guard<mutex> lock(mutex_);
                functors.swap(pendingFunctors_);//交换两个vector内部底层数组指针->交换后,pendingFunctors_变成空,functors:[任务 A，任务 B，任务 C]
            }

            for(auto& func:functors) func();
            //checkConnectionTimeout();
    }
    LOG_WARN<<"EventLoop退出";
}
void EventLoop::addChannel(Channel* channel){
    channels_[channel->fd()]=channel;
    epoller_.addFd(channel->fd(),channel->events());
    LOG_INFO<<"添加Channel成功 fd="<<channel->fd();
}
void EventLoop::updateChannel(Channel* channel){
    int fd=channel->fd();
    if(channels_.find(fd)==channels_.end()){
        // 不在map = 还没加入epoll，执行add
        channels_[fd]=channel;
        epoller_.addFd(fd,channel->events());
        LOG_INFO<<"Channel不存在,新增注册 fd="<<fd;
    }else{  // 已经存在，执行modify
        epoller_.modifyFd(fd, channel->events());
        LOG_INFO<<"更新Channel事件 fd="<<fd;
    }
}
void EventLoop::removeChannel(Channel* channel){
    int fd=channel->fd();
    epoller_.removeFd(fd);
    channels_.erase(fd);
    LOG_INFO<<"移除Channel fd="<<fd;
}
//注册时设置时间回调
void EventLoop::setTimerCallback(std::function<void()> cb){
    timerCallback_=cb;//timerCallback_存函数对象的私有成员变量
    LOG_INFO<<"设置定时器回调成功";
}
void EventLoop::addConnection(int fd,TcpConnection* conn){
    connections_[fd]=conn;
    LOG_INFO<<"添加TcpConnection fd="<<fd;
}

void EventLoop::queueInLoop(std::function<void()> cb){
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pendingFunctors_.push_back(cb);
    }
    LOG_INFO<<"添加跨线程任务成功";
    wakeup();
}
void EventLoop::checkConnectionTimeout(){
    LOG_INFO<<"开始检查连接超时 当前连接数量="<<connections_.size();
    vector<int> timeoutFds;

    for(auto& item:connections_){
        TcpConnection* conn=item.second;
        LOG_INFO<<"检查连接 fd="<<item.first<<" username="<<conn->getUsername();

        if(conn->isTimeout()){
            LOG_WARN<<"发现超时连接 fd="<<item.first<<" username="<<conn->getUsername();
            timeoutFds.push_back(item.first);
        }
    }

    for(int fd:timeoutFds){
        auto it=connections_.find(fd);
        if(it!=connections_.end()){
            TcpConnection* conn=it->second;
            LOG_WARN<<"关闭超时连接 fd="<<fd<<" username="<<conn->getUsername();
            conn->handleClose();
        }
    }
}
void EventLoop::deleteConnection(int fd){
    auto it=connections_.find(fd);
    if(it!=connections_.end()){
        TcpConnection* conn=it->second;
        connections_.erase(it);
        delete conn;
        LOG_INFO<<"删除TcpConnection fd="<<fd;
    }
}
bool EventLoop::isInLoopThread(){
    return threadId_==std::this_thread::get_id();
}
void EventLoop::wakeup(){
    uint64_t one=1;
    write(wakeupFd_,&one,sizeof(one));
    LOG_INFO<<"唤醒EventLoop";
}
void EventLoop::handleWakeup(){
    uint64_t one;
    read( wakeupFd_,&one,sizeof(one));
    LOG_INFO<<"处理EventLoop唤醒事件";
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
