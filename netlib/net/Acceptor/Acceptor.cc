#include "Acceptor.h"
#include "../Channel/Channel.h"
#include "../../base/Logger/Logger.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>

using namespace std;
Acceptor::Acceptor(EventLoop& loop,const std::string& ip,int port):
loop_(loop),ip_(ip),port_(port),
listen_fd_(-1),channel_(NULL){
    LOG_INFO<<"创建Acceptor成功 ip="<<ip_<<" port="<<port_;
}
Acceptor::~Acceptor(){
    LOG_INFO<<"销毁Acceptor";
    if(channel_) delete channel_;
    if(listen_fd_!=-1) close(listen_fd_);
}
void Acceptor::start(){
    //1socket
    LOG_INFO<<"开始创建监听socket";
    listen_fd_=socket(AF_INET,SOCK_STREAM,0);
    if(listen_fd_<0){
      LOG_ERROR<<"socket创建失败";
      exit(-1);
    }
    LOG_INFO<<"socket创建成功 listen_fd="<<listen_fd_;
    //地址复用
    int opt=1;
    setsockopt(listen_fd_,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
    //非阻塞
    int flags=fcntl(listen_fd_,F_GETFL,0);
    fcntl(listen_fd_,F_SETFL,flags|O_NONBLOCK);
    LOG_INFO<<"设置监听socket非阻塞成功 fd="<<listen_fd_;
    //2bind
    sockaddr_in addr{};
    int n=inet_pton(AF_INET,ip_.c_str(),&addr.sin_addr);//字符串转二进制
    if(n<=0){
        LOG_ERROR<<"inet_pton地址转换失败 ip="<<ip_;
        return ;
    }
    addr.sin_family=AF_INET;
    addr.sin_port=htons(port_);
    
    n=bind(listen_fd_,(sockaddr*)&addr,sizeof(addr));
    if(n<0){
        LOG_ERROR<<"bind绑定失败 ip="<<ip_<<" port="<<port_;
        return ;
    }
    LOG_INFO<<"bind绑定成功";
    //3listen
    n=listen(listen_fd_,10);
    if(n<0){
       LOG_ERROR<<"listen监听失败";
       return;
    }  

    LOG_INFO<<"服务器监听成功 ip="<<ip_<<" port="<<port_<<" listen_fd="<<listen_fd_;
    //将listen_fd添加到EventLoop,因为EventLoop直接管理的是Channel

    //创建卡片(创建Channel),绑定fd和EventLoop
    channel_=new Channel(&loop_,listen_fd_);
    //在卡片上记录,回调函数,当满足某个条件时跑这个函数(对listen_fd而言就是client_fd请求连接)
    //setReadCallback需要传入一个处理函数，这个函数代表当该fd发生EPOLLIN事件时应该执行什么操作。
    //对于listen_fd，这个函数通常是Acceptor::handleRead；对于client_fd，这个函数通常是TcpConnection::handleRead。
    channel_->setReadCallback(std::bind(&Acceptor::handleRead,this));//如果事件触发了，就执行
    
    LOG_INFO<<"设置监听socket读回调成功 fd="<<listen_fd_;

    //这就是写下了某个条件是什么
    channel_->enableReading(); //决定了是什么事件

    LOG_INFO<<"监听socket加入EventLoop成功 fd="<<listen_fd_;
}
void Acceptor::handleRead(){//setReadCallback中参数说明了"发生某种事件后，要执行的函数"->listen_fd:client_fd请求
    LOG_INFO<<"开始处理客户端连接请求";
    while(1){
    sockaddr_in addr{};
    socklen_t len=sizeof(addr);
    int client_fd=accept(listen_fd_,(sockaddr*)&addr,&len);
    if(client_fd < 0){
        if(errno==EAGAIN ||errno==EWOULDBLOCK){
        //连接已经全部取完
        LOG_INFO<<"当前连接已经全部接收完成";
        break;
    }   if(errno==EINTR){
        //信号中断，继续accept
        LOG_WARN<<"accept被信号中断,继续接收";
        continue;
    }
        LOG_ERROR<<"accept接收客户端失败";
        break;    
    }
    LOG_INFO<<"接收到新客户端 fd="<<client_fd;
    //设置client_fd非阻塞
    int flags = fcntl(client_fd,F_GETFL,0);
    fcntl(client_fd,F_SETFL,flags | O_NONBLOCK);

    LOG_INFO<<"设置客户端非阻塞成功 fd="<<client_fd;

    //通知TcpServer
    if(newConnectionCallback_){
        LOG_INFO<<"通知TcpServer创建连接 fd="<<client_fd;
         newConnectionCallback_(client_fd);
        }else{
        LOG_WARN<<"未设置新连接回调 fd="<<client_fd;
        }
    }
}
void Acceptor::setNewConnectionCallback(std::function<void(int)>cb){
    newConnectionCallback_=cb;
    LOG_INFO<<"设置新连接回调成功";
}