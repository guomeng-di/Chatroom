#include "Epoller.h"
#include <unistd.h>
#include "../../base/Logger.h"
#include <iostream>
using namespace std;
Epoller::Epoller(){
    epfd_=epoll_create1(0);
    //printf("epfd_ = %d\n", epfd_);
    Logger::instance().info("epfd="+to_string(epfd_));
    if(epfd_<0){
        //perror("epoll_create");
        Logger::instance().error("epoll_create failed");
        exit(1);
    }
    events_.resize(1024);
}
Epoller::~Epoller(){
    close(epfd_);
}
int Epoller::wait(int timeout){
    int n=epoll_wait(epfd_,events_.data(),events_.size(),timeout);
    if(n<0){
       //perror("epoll_wait");
       Logger::instance().error("epoll_wait failed");
       return -1;
    }
    // cout<<"epoll return n="
    //     <<n
    //     <<endl;
    Logger::instance().info("epoll return n="+to_string(n));
      if(n>0)
    {
        for(int i=0;i<n;i++)
        {
            // cout<<"fd="
            //     <<events_[i].data.fd
            //     <<" events="
            //     <<events_[i].events
            //     <<endl;
            Logger::instance().info("fd="+to_string(events_[i].data.fd)+" events="+to_string(events_[i].events));
        }
    }
    return n;
}
void Epoller::addFd(int fd,uint32_t events){
    struct epoll_event ev{};
    ev.data.fd=fd;
    ev.events=events;
    //printf("Epoller::addFd epfd=%d, target_fd=%d\n", epfd_, fd);
    Logger::instance().info("Epoller::addFd epfd="+to_string(epfd_)+" target_fd="+to_string(fd));
    int n=epoll_ctl(epfd_,EPOLL_CTL_ADD,fd,&ev);
    if(n<0){
        //perror("epoll_ctl");
        Logger::instance().error("epoll_ctl ADD failed");
        return ;
    }
}
void Epoller::removeFd(int fd){
    int n=epoll_ctl(epfd_,EPOLL_CTL_DEL,fd,NULL);
    if(n<0){
        //perror("epoll_ctl");
        Logger::instance().error("epoll_ctl DEL failed");
        return ;
    }
}
void Epoller::modifyFd(int fd,uint32_t events){
    epoll_event ev{};
    ev.data.fd=fd;
    ev.events=events;
    int n=epoll_ctl(epfd_,EPOLL_CTL_MOD,fd,&ev);
    if(n<0){
        //perror("epoll_ctl");
        Logger::instance().error("epoll_ctl MOD failed");
        return ;
    }
}
int Epoller::getEventFd(int x){
        return events_[x].data.fd;
}
uint32_t Epoller::getEvents(int x){
        return events_[x].events;
}
