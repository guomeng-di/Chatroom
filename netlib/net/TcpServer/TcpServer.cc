#include "TcpServer.h"
#include "../Acceptor/Acceptor.h"
#include "../TcpConnection/TcpConnection.h"
#include "../EventLoop/EventLoop.h"
#include "../EventLoopThreadPool/EventLoopThreadPool.h"
#include "../../base/Logger.h"
#include <functional>
using namespace std;

TcpServer::TcpServer(EventLoop& loop,const string& ip,int port):
loop_(loop),ip_(ip),port_(port),acceptor_(new Acceptor(loop_,ip_,port)),threadPool_(new EventLoopThreadPool(&loop_,4)){
    acceptor_->setNewConnectionCallback(
        bind(&TcpServer::newConnection,this,placeholders::_1)
    );
}
TcpServer::~TcpServer(){
    delete acceptor_;
    delete threadPool_;
}
void TcpServer::start(){
    //启动sub reactor线程
    threadPool_->start();
    //启动accept
    acceptor_->start();
}

void TcpServer::newConnection(int client_fd){
    Logger::instance().info("new client fd=" +to_string(client_fd));
    EventLoop* ioLoop=threadPool_->getNextLoop();
    cout<<"========== NEW CONNECTION =========="<<endl;
    cout<<"client fd="<<client_fd<<endl;
    cout<<"selected EventLoop="<<ioLoop<<endl;
    cout<<"===================================="<<endl;
    ioLoop->queueInLoop(
        [ioLoop,client_fd](){
            TcpConnection* conn=new TcpConnection(ioLoop,client_fd);
            ioLoop->addConnection(client_fd,conn);
            cout<<"========== ADD CONNECTION =========="<<endl;
            cout<<"fd="<<client_fd<<endl;
            cout<<"EventLoop="<<ioLoop<<endl;
            cout<<"===================================="<<endl;
        }
    );
}
