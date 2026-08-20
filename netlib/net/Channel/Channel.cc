#include "Channel.h"
#include "../EventLoop/EventLoop.h"
#include "../../base/Logger.h"
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
    //如果发生的事件包含可读,就调用注册好的可读事件的回调函数
    if(revents_ & (EPOLLERR | EPOLLHUP)){//EPOLLERR:socket错误,EPOLLHUP:对端关闭连接
        Logger::instance().error("EPOLLERR or EPOLLHUP fd="+to_string(fd_));
        if(closeCallback_) closeCallback_();
        return;
    }
    if(revents_&EPOLLIN){
            Logger::instance().info("EPOLLIN");
            if(readCallback_){
                Logger::instance().info("call readCallback");
                readCallback_();
            }else{
                Logger::instance().info("readCallback empty");
        }
    }
    if(revents_ & EPOLLOUT){
        if(writeCallback_) writeCallback_();
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