#include "Acceptor.h"
#include "../Channel/Channel.h"
#include "../../base/Logger.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>

using namespace std;
Acceptor::Acceptor(EventLoop& loop,const std::string& ip,int port):
loop_(loop),ip_(ip),port_(port),
listen_fd_(-1),channel_(NULL){
}
Acceptor::~Acceptor(){
    if(channel_) delete channel_;
    if(listen_fd_!=-1) close(listen_fd_);
}
void Acceptor::start(){
    //1socket
    listen_fd_=socket(AF_INET,SOCK_STREAM,0);
    if(listen_fd_<0){
        //perror("socket create failed");
        Logger::instance().error("socket create failed");
        exit(-1);
    }
    printf("listen_fd = %d\n", listen_fd_);
    //2bind
    sockaddr_in addr{};
    int n=inet_pton(AF_INET,ip_.c_str(),&addr.sin_addr);//字符串转二进制
    if(n<=0){
        //perror("inet_pton");
        Logger::instance().error("inet_pton failed");
        return ;
    }
    addr.sin_family=AF_INET;
    addr.sin_port=htons(port_);
    
    n=bind(listen_fd_,(sockaddr*)&addr,sizeof(addr));
    if(n<0){
        //perror("bind");
        Logger::instance().error("bind failed");
        return ;
    }
    //3listen
    n=listen(listen_fd_,10);
    if(n<0){
       //perror("listen");
       Logger::instance().error("listen failed");
        return ;
    }  

    //cout<<"server listen on "<<ip_<<":"<<port_<<endl;
    Logger::instance().info("server listen on "+ip_+":"+to_string(port_));
    //socket拿到合法fd之后，再创建Channel
    channel_=new Channel(&loop_,listen_fd_);
    //将listen_fd添加到EventLoop,因为EventLoop直接管理的是Channel
    //所以先创建Channel
    //告诉Channel处理函数,补充Channel
    channel_->setReadCallback(
        std::bind(&Acceptor::handleRead,this)
    );
    //setReadCallback需要传入一个处理函数，这个函数代表当该fd发生EPOLLIN事件时应该执行什么操作。对于listen_fd，这个函数通常是Acceptor::handleRead；对于client_fd，这个函数通常是TcpConnection::handleRead。
    //通知EventLoop,EventLoop记录Channel
    channel_->enableReading();  
}
void Acceptor::handleRead(){//setReadCallback中参数说明了"发生某种事件后，要执行的函数",由该函数产生
    sockaddr_in addr{};
    socklen_t len=sizeof(addr);
    int client_fd=accept(listen_fd_,(sockaddr*)&addr,&len);
    if(client_fd < 0){
        //perror("accept");
        Logger::instance().error("accept failed");
        return ;    
    }
    //通知TcpServer
    if(newConnectionCallback_)
         newConnectionCallback_(client_fd);

}
void Acceptor::setNewConnectionCallback(std::function<void(int)>cb){
    newConnectionCallback_=cb;
}