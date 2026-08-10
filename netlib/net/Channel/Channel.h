// EventLoop检测到事件发生
//     |
//     |
//  Channel把fd和处理函数绑定起来,作为后续分流判断依据
//     |
//     |
// TcpServer / TcpConnection

//思考:Channel需要保存什么?
//一个 Channel 对象代表：一个fd+监听哪些事件+发生事件后调用哪个函数
//1fd 2监听事件(EPOLLIN读/EPOLLOUT写)  3回调函数(readCallback_/writeCallback_/closeCallback_)
#pragma once
#include <functional>
#include <cstdint>
#include <sys/epoll.h>

class EventLoop;

class Channel{
    public:
      Channel(EventLoop* loop,int fd);
      ~Channel();

      int fd();//获取fd(fd是private,通过函数封装传出去)
      uint32_t events();
      void setRevents(uint32_t revents);//Epoll_wait产生revents
      void enableReading();//通知EventLoop
      void handleEvent();//判断执行哪个函数
      void setReadCallback(std::function<void()> readCallback);
      void setWriteCallback(std::function<void()> writeCallback);
      void setCloseCallback(std::function<void()> closeCallback);
    private:
      int fd_;
      EventLoop* loop_;//(Channel改变监听事件时，需要通知EventLoop)
      uint32_t events_;//对于这个fd,我关心的事
      uint32_t revents_;//发生的事
      std::function<void()> readCallback_;//事件
      std::function<void()> writeCallback_;
      std::function<void()> closeCallback_;
};
// fd是标识
// events_是对于这个fd我关心什么，也就是fd可能发生什么
// revents_是实际上发生的事
// 通过revents_,handleEvent()决定调用哪个回调函数
// 发生事件	调用
// EPOLLIN	readCallback
// EPOLLOUT	writeCallback
// 断开连接	closeCallback
// listen_fd和client_fd的区别不是在callback,而是它们绑定的callback内容不同