#include "TcpConnection.h"
#include "../Channel/Channel.h"
#include "../EventLoop/EventLoop.h"
#include "../Buffer/Buffer.h"
#include "../../../protocol/JsonProtocol/JsonProtocol.h"
#include "../../../protocol/MessageCodec/MessageCodec.h"
#include "../../../service/MessageDispatcher/MessageDispatcher.h"
#include <sys/socket.h>
#include "../../../service/FileService/FileService.h"
#include "../../../protocol/MsgId.h"

#include "../../../manager/OnlineUserManager/OnlineUserManager.h"
#include <unistd.h>
#include <iostream>
#include "../../../manager/RedisManager/RedisManager.h"

#include <cstring>
#include "../../base/Logger.h"
using namespace std;
TcpConnection::TcpConnection(EventLoop* loop,int fd):
fd_(fd),loop_(loop),channel_(new Channel(loop_,fd_)){
    lastActiveTime_=time(nullptr);
    Logger::instance().info("TcpConnection create fd="+to_string(fd_)
);
    channel_->setReadCallback(
    std::bind(&TcpConnection::handleRead,this)
);
//cout<<"set read callback"<<endl;
Logger::instance().info(
    "set read callback"
); 
channel_->enableReading();
}
TcpConnection::~TcpConnection(){
    // 释放channel
    if(channel_) delete channel_;
    if(fd_!=-1) close(fd_);
}
void TcpConnection::handleRead(){
    Logger::instance().info("TcpConnection handleRead called");
    char buf[1024];
    int n=recv(fd_,buf,sizeof(buf),0);
    if(n>0){
   
    //登录、聊天、好友操作都会刷新活跃时间
    updateActiveTime();
    Logger::instance().info("recv bytes="+to_string(n));

    string recvData(buf,n);
    buffer_.append(buf,n);

    while(buffer_.hasMessage()){ 
        string msg=buffer_.retrieveMessage();
        int msgid=MessageCodec::getMsgId(msg);

        if(msgid==FILE_DATA_MSG){
            //FileService::receiveFileData(msg,this);

            FilePacket packet=MessageCodec::decodeBinary(msg);
            cout<<"receive file block"<<endl;
            cout<<"filename="<<packet.info["filename"]<<endl;
            cout<<"blockid="<<packet.info["blockid"]<<endl;
            cout<<"data size="<<packet.data.size()<<endl;

            FileService::receiveFileData(packet,this);


        }else{
            json js=JsonProtocol::decode(msg);
            MessageDispatcher::dispatch(js,this);
        }
    }
}else if(n==0) handleClose();
    else{
        //perror("recv");
        Logger::instance().error("recv failed");
        handleClose();
    }
}
void TcpConnection::send(const string& msg){
    Logger::instance().info("TcpConnection send:"+msg);
    string data=MessageCodec::encode(msg);
    Logger::instance().info("send bytes="+to_string(data.size()));

    int n=::send(fd_,data.c_str(),data.size(),0);
    Logger::instance().info("send return="+to_string(n));
    if(n<0){
        //perror("send");
        Logger::instance().error("send failed");
    }
   
}
void TcpConnection::handleClose(){
    //cout<<"client close fd="<<fd_<<endl;
    Logger::instance().info(
    "client close fd="+to_string(fd_));
    //OnlineUserManager onlineUserManager;
    if(!username_.empty()){
        OnlineUserManager::instance().removeUser(username_);
        // cout<<"remove online user:"
        //     <<username_
        //     <<endl;

        if(RedisManager::instance().connect()){ RedisManager::instance().setOffline(username_);
        Logger::instance().info("remove online user:"+username_);
    }}
    loop_->removeChannel(channel_);
    //close(fd_);
    fd_=-1;
}
void TcpConnection::setUsername(const string& username){
    username_=username;
}
string TcpConnection::getUsername(){
    return username_;
}
void TcpConnection::updateActiveTime(){
    lastActiveTime_=time(nullptr);
}
bool TcpConnection::isTimeout(){
    time_t now=time(nullptr);
    return now-lastActiveTime_>60?1:0;
}