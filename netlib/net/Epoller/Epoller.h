//思考:一个epoll对象需要保存什么
//fd+对应发生的事件
//1epoll文件描述符epfd(由epoll_create创建)
//2添加事件函数addFd(socket加入epoll)
//3删除事件函数removeFd(关闭客户端)
//4等待事件函数wait_(epoll_wait)
#pragma once
#include <vector>
#include <sys/epoll.h>

class Epoller{
    public:
      Epoller();
      ~Epoller();

      int wait(int timeout);
      void addFd(int fd,uint32_t events);
      void removeFd(int fd);
      void modifyFd(int fd,uint32_t events);
      int getEventFd(int x);//等待的事件的fd
      uint32_t getEvents(int x);//状态(EPOLLIN/..)

    private:
      int epfd_;
      std::vector<epoll_event> events_;
};