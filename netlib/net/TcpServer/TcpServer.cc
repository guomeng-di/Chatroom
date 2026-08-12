#include "TcpServer.h"

#include "../Acceptor/Acceptor.h"
#include "../TcpConnection/TcpConnection.h"
#include "../EventLoop/EventLoop.h"
#include "../../base/Logger.h"
#include <functional>


using namespace std;
TcpServer::TcpServer(EventLoop& loop,const std::string& ip,int port):
loop_(loop),ip_(ip),port_(port),acceptor_(new Acceptor(loop_,ip_,port_)){
    acceptor_->setNewConnectionCallback(
        bind(&TcpServer::newConnection, this,std::placeholders::_1)//有一个参数
    );
    loop_.setTimerCallback(bind(&TcpServer::checkConnectionTimeout,this));//loop的时间到了就调用我提前写好的函数:checkConnectionTimeout
    Logger::instance().info("timer callback set");
}
TcpServer::~TcpServer(){
    for(auto& conn:connections_) delete conn.second;
    connections_.clear();
    delete acceptor_;
}
void TcpServer::start(){//acceptor拿到client_fd后,执行TcpServer留的回调函数
    acceptor_->start();
}
void TcpServer::newConnection(int client_fd){
    //cout<<"new client fd="<<client_fd<<endl;
    Logger::instance().info("new client fd="+ to_string(client_fd));
    TcpConnection* conn=new TcpConnection(&loop_,client_fd);
    connections_[client_fd]=conn;
}
void TcpServer::checkConnectionTimeout(){
    Logger::instance().info("check timeout");
    for(auto it=connections_.begin();it!=connections_.end();){
        TcpConnection* conn=it->second;
        Logger::instance().info( "check fd="+to_string(it->first));
        if(conn->isTimeout()){
            Logger::instance().error("connection timeout");
            //conn->handleClose();
            //it=connections_.erase(it);
            continue;
        }else ++it;
    }
}
