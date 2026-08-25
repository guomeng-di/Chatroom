#include "Channel.h"
#include "../EventLoop/EventLoop.h"
#include "../../base/Logger/Logger.h"
#include <iostream>
using namespace std;

Channel::Channel(EventLoop* loop,int fd):
fd_(fd),loop_(loop),events_(0),revents_(0){
    LOG_INFO<<"创建Channel fd="<<fd_;
}
Channel::~Channel(){
    LOG_INFO<<"销毁Channel fd="<<fd_;
}
int Channel::fd(){
    return fd_;
}
uint32_t Channel::events(){
    return events_;
}
void Channel::setRevents(uint32_t revents){
    revents_=revents;
    LOG_INFO<<"设置Channel发生事件 fd="<<fd_<<" events="<<revents_;
}
void Channel::enableReading(){
    events_|=EPOLLIN;
    LOG_INFO<<"开启读事件监听 fd="<<fd_<<" events="<<events_;
    loop_->updateChannel(this);
}
void Channel::handleEvent(){
    LOG_INFO<<"Channel处理事件 fd="<<fd_;

    // 可读回调可能关闭并释放 TcpConnection。使用本地事件掩码，并在读回调后停止分发，
    // 避免访问已释放的 Channel 或重复调用写/关闭回调。
    const uint32_t revents = revents_;

    if(revents & EPOLLERR){
        LOG_ERROR<<"检测到EPOLLERR错误 fd="<<fd_;
        if(closeCallback_){
            closeCallback_();
        }
        return;
    }
    if(revents & EPOLLHUP){
        LOG_WARN<<"检测到EPOLLHUP连接挂断 fd="<<fd_;
    }

    EventLoop* loop=loop_;
    Channel* self=this;

    if(revents & EPOLLOUT){
        LOG_INFO<<"检测到可写事件EPOLLOUT fd="<<fd_;

        if(writeCallback_){
            LOG_INFO<<"执行写回调 fd="<<fd_;
            writeCallback_();
        }
        // 写回调发生错误时可能关闭并释放当前连接，不能继续访问已释放的 Channel。
        if(!loop->hasChannel(fd_,self)) return;
    }

    if(revents & EPOLLIN){
        LOG_INFO<<"检测到可读事件EPOLLIN fd="<<fd_;

        if(readCallback_){
            LOG_INFO<<"执行读回调 fd="<<fd_;
            readCallback_();
        }
        return;
    }

    if((revents & EPOLLHUP) && closeCallback_){
        LOG_WARN<<"执行关闭回调 fd="<<fd_;
        closeCallback_();
        return;
    }
}
void Channel::setReadCallback(function<void()> cb){
    readCallback_=cb;
    LOG_INFO<<"设置读事件回调 fd="<<fd_;
}
void Channel::setWriteCallback(function<void()> cb){
    writeCallback_=cb;
    LOG_INFO<<"设置写事件回调 fd="<<fd_;
}
void Channel::setCloseCallback(function<void()> cb){
    closeCallback_=cb;
    LOG_INFO<<"设置关闭事件回调 fd="<<fd_;
}
void Channel::enableWriting(){
    events_ |= EPOLLOUT;
    LOG_INFO<<"开启写事件监听 fd="<<fd_<<" events="<<events_;
    loop_->updateChannel(this);
}
void Channel::disableWriting(){
    events_ &= ~EPOLLOUT;
    LOG_INFO<<"关闭写事件监听 fd="<<fd_<<" events="<<events_;
    loop_->updateChannel(this);
}
