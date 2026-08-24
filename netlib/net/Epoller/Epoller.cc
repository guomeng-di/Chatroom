#include "Epoller.h"
#include <unistd.h>
#include "../../base/Logger/Logger.h"
#include <iostream>
using namespace std;
Epoller::Epoller(){
    epfd_=epoll_create1(0);
    if(epfd_<0){
        LOG_ERROR<<"创建epoll失败";
        exit(1);
    }
    LOG_INFO<<"创建epoll成功 epfd="<<epfd_;
    events_.resize(1024*4);
}
Epoller::~Epoller(){
    LOG_INFO<<"关闭epoll epfd="<<epfd_;
    close(epfd_);
}
int Epoller::wait(int timeout){
    int n=epoll_wait(epfd_,events_.data(),events_.size(),timeout);
    if(n<0){
       LOG_ERROR<<"epoll_wait执行失败";
       return -1;
    }
LOG_INFO<<"epoll等待返回事件数量="<<n;
      if(n>0){
        for(int i=0;i<n;i++){
            LOG_INFO<<"检测到事件 fd="<<events_[i].data.fd<<" events="<<events_[i].events;
        }
    }
    return n;
}
void Epoller::addFd(int fd,uint32_t events){
    struct epoll_event ev{};
    ev.data.fd=fd;
    ev.events=events;
     LOG_INFO<<"添加fd到epoll"<<" epfd="<<epfd_<<" fd="<<fd<<" events="<<events;
    int n=epoll_ctl(epfd_,EPOLL_CTL_ADD,fd,&ev);
    if(n<0){
        LOG_ERROR<<"epoll_ctl添加失败 fd="<<fd;
        return ;
    }
    LOG_INFO<<"epoll_ctl添加成功 fd="<<fd;
}
void Epoller::removeFd(int fd){
    int n=epoll_ctl(epfd_,EPOLL_CTL_DEL,fd,NULL);
    if(n<0){
        LOG_ERROR<<"epoll_ctl删除失败 fd="<<fd;
        return ;
    }
    LOG_INFO<<"epoll_ctl删除成功 fd="<<fd;
}
void Epoller::modifyFd(int fd,uint32_t events){
    epoll_event ev{};
    ev.data.fd=fd;
    ev.events=events;
    int n=epoll_ctl(epfd_,EPOLL_CTL_MOD,fd,&ev);
    if(n<0){
        LOG_ERROR<<"epoll_ctl修改失败 fd="<<fd;
        return ;
    }
    LOG_INFO<<"epoll修改fd事件成功 fd="<<fd<<" events="<<events;
}
int Epoller::getEventFd(int x){
        return events_[x].data.fd;
}
uint32_t Epoller::getEvents(int x){
        return events_[x].events;
}
