#include "Channel.h"
#include "../EventLoop/EventLoop.h"
#include "../../base/Logger.h"
#include <iostream>
using namespace std;

Channel::Channel(EventLoop* loop,int fd):
fd_(fd),loop_(loop),events_(0),revents_(0){

}
Channel::~Channel(){
}
int Channel::fd(){
    return fd_;
}
uint32_t Channel::events(){
    return events_;
}
void Channel::setRevents(uint32_t revents){
    revents_=revents;
}
void Channel::enableReading(){
    events_|=EPOLLIN;
    Logger::instance().info("Channel::enableReading fd="+to_string(fd_)+", events="+to_string(events_));
    loop_->updateChannel(this);
}
void Channel::handleEvent(){
    Logger::instance().info("Channel handleEvent fd="+to_string(fd_));

    // 可读回调可能关闭并释放 TcpConnection。使用本地事件掩码，并在读回调后停止分发，
    // 避免访问已释放的 Channel 或重复调用写/关闭回调。
    const uint32_t revents = revents_;

    if(revents & EPOLLERR){
        //cout<<"[Channel] EPOLLERR -> closeCallback"<<endl;
        if(closeCallback_){
            closeCallback_();
        }

        return;
    }

    if(revents & EPOLLHUP){
        //cout<<"[Channel] EPOLLHUP detected"<<endl;
    }

    if(revents & EPOLLIN){
        Logger::instance().info("EPOLLIN");

        if(readCallback_){
            Logger::instance().info("call readCallback");
            readCallback_();
        }
        return;
    }

    if(revents & EPOLLOUT){
        if(writeCallback_){
            writeCallback_();
        }
    }

    if((revents & EPOLLHUP) && closeCallback_){
        //cout<<"[Channel] EPOLLHUP -> closeCallback"<<endl;
        closeCallback_();
        return;
    }
}
void Channel::setReadCallback(function<void()> cb){
    readCallback_=cb;
}
void Channel::setWriteCallback(function<void()> cb){
    writeCallback_=cb;
}
void Channel::setCloseCallback(function<void()> cb){
    closeCallback_=cb;
}
void Channel::enableWriting(){
    events_ |= EPOLLOUT;
    loop_->updateChannel(this);
}
void Channel::disableWriting(){
    events_ &= ~EPOLLOUT;
    loop_->updateChannel(this);
}
