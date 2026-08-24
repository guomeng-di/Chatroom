#include "TcpServer.h"
#include "../Acceptor/Acceptor.h"
#include "../TcpConnection/TcpConnection.h"
#include "../EventLoop/EventLoop.h"
#include "../EventLoopThreadPool/EventLoopThreadPool.h"
#include "../../base/Logger/Logger.h"
#include <functional>
using namespace std;

TcpServer::TcpServer(EventLoop& loop,const string& ip,int port):
loop_(loop),ip_(ip),port_(port),acceptor_(new Acceptor(loop_,ip_,port)),threadPool_(new EventLoopThreadPool(&loop_,4)){
    
    LOG_INFO<<"创建TcpServer成功"<<" ip="<<ip_<<" port="<<port_;
    acceptor_->setNewConnectionCallback(//设置 “收到新客户端连接” 时要调用的回调函数
        bind(&TcpServer::newConnection,this,placeholders::_1)
    );
    LOG_INFO << "设置新连接回调成功";
}
TcpServer::~TcpServer(){
    LOG_INFO << "TcpServer析构";
    delete acceptor_;
    delete threadPool_;
}
void TcpServer::start(){
    //启动sub reactor线程
    LOG_INFO << "启动Sub Reactor线程池";
    threadPool_->start();
    LOG_INFO << "Sub Reactor线程池启动完成";
    //启动accept
    LOG_INFO << "启动Acceptor监听";
    acceptor_->start();
    LOG_INFO << "服务器开始监听"<< " ip=" << ip_<< " port=" << port_;
}

void TcpServer::newConnection(int client_fd){
    LOG_INFO << "收到新的客户端连接"<< " fd=" << client_fd;
    EventLoop* ioLoop=threadPool_->getNextLoop();
    if(ioLoop==nullptr){
        LOG_ERROR << "获取Sub Reactor失败"<< " fd=" << client_fd;
        close(client_fd);
        return;
    }
    LOG_INFO << "分配Sub Reactor成功"<< " fd=" << client_fd<< " EventLoop=" << ioLoop;
    ioLoop->queueInLoop(
        [ioLoop,client_fd](){
            LOG_INFO << "创建TcpConnection"<< " fd=" << client_fd;
            TcpConnection* conn=new TcpConnection(ioLoop,client_fd);
            ioLoop->addConnection(client_fd,conn);
             LOG_INFO << "客户端连接注册到Sub Reactor成功"<< " fd=" << client_fd<< " EventLoop=" << ioLoop;
        }
    );
}
