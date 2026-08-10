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
    //printf("Channel::enableReading fd=%d, events=%u\n", fd_, events_);
    Logger::instance().info("Channel::enableReading fd="+to_string(fd_)+", events="+to_string(events_));
    loop_->updateChannel(this);
}
void Channel::handleEvent(){
    //  cout<<"Channel handleEvent fd="
    //     <<fd_
    //     <<endl;
    Logger::instance().info("Channel handleEvent fd="+to_string(fd_));
         if(revents_&EPOLLIN){
        //cout<<"EPOLLIN"<<endl;
        Logger::instance().info("EPOLLIN");

        if(readCallback_){
            // cout<<"call readCallback"<<endl;
            Logger::instance().info("call readCallback");
            readCallback_();
        }else{
            // cout<<"readCallback empty"<<endl;
            Logger::instance().info("readCallback empty");
        }
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
