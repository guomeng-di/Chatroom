#include "TcpConnection.h"
#include "../Channel/Channel.h"
#include "../EventLoop/EventLoop.h"
#include "../Buffer/Buffer.h"
#include "../../../protocol/JsonProtocol/JsonProtocol.h"
#include "../../../protocol/MessageCodec/MessageCodec.h"
#include "../../../service/MessageDispatcher/MessageDispatcher.h"
#include <sys/socket.h>
#include <errno.h>
#include "../../../service/FileService/FileService.h"
#include "../../../protocol/MsgId.h"
#include "../../../netlib/base/SocketUtil/SocketUtil.h"
#include "../../../manager/OnlineUserManager/OnlineUserManager.h"
#include <unistd.h>
#include <iostream>
#include "../../../manager/RedisManager/RedisManager.h"

#include <cstring>
#include "../../base/Logger.h"
using namespace std;
TcpConnection::TcpConnection(EventLoop* loop,int fd):
fd_(fd),loop_(loop),channel_(new Channel(loop_,fd_)),connected_(true){
    lastActiveTime_=time(nullptr);

    Logger::instance().info("TcpConnection create fd="+to_string(fd_));
    Logger::instance().info("set read callback"); 
    channel_->setReadCallback(std::bind(&TcpConnection::handleRead,this));
    channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite,this));
    channel_->setCloseCallback(std::bind(&TcpConnection::handleClose,this));
channel_->enableReading();
}
TcpConnection::~TcpConnection(){
    // 释放channel
    if(channel_) delete channel_;
    if(fd_!=-1) close(fd_);
}
void TcpConnection::handleRead(){
    Logger::instance().info("TcpConnection handleRead called");
    char buf[1024*4];
while(1){
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
        try{
            if(msgid==FILE_DATA_MSG){
                FilePacket packet= MessageCodec::decodeBinary(msg);
                FileService::receiveFileData(packet,this);
            }else{
                json js=JsonProtocol::decode(msg);
                MessageDispatcher::dispatch(js,this);
            }
        }catch(const exception& e){
             Logger::instance() .error(string("message error:")+e.what());
             handleClose();
             return;
            }
        }

        if(buffer_.hasError()){
            Logger::instance().error("invalid packet size");
            handleClose();
            return ;
        }
}else if(n==0){
            //客户端关闭连接
            handleClose();
            return;
}else{//n<0
    if(errno==EAGAIN ||errno==EWOULDBLOCK) break;//非阻塞正常情况
    if(errno==EINTR) continue; //信号打断，继续读
    Logger::instance().error("recv error");
    handleClose();
    return;

        // if(msgid==FILE_DATA_MSG){
        //     //FileService::receiveFileData(msg,this);

        //     FilePacket packet=MessageCodec::decodeBinary(msg);
        //     cout<<"receive file block"<<endl;
        //     cout<<"filename="<<packet.info["filename"]<<endl;
        //     cout<<"blockid="<<packet.info["blockid"]<<endl;
        //     cout<<"data size="<<packet.data.size()<<endl;

        //     FileService::receiveFileData(packet,this);


        // }else{
        //     json js=JsonProtocol::decode(msg);
        //     MessageDispatcher::dispatch(js,this);
        // }
//     }
// }else if(n==0) handleClose();
//     else{
//         //perror("recv");
//         Logger::instance().error("recv failed");
//         handleClose();
//     }
}
}
}
void TcpConnection::send(const string& msg){
    Logger::instance().info("TcpConnection send");
    loop_->queueInLoop(
        [this,msg](){
            string data=MessageCodec::encode(msg);
            outputBuffer_.append(data.data(),data.size());
            channel_->enableWriting();
        }
    );
}
bool TcpConnection::sendBinary(const string& msg){
    Logger::instance().info("TcpConnection send binary");
    //int n=::send(fd_,data.c_str(),data.size(),0);
    bool ok=SocketUtil::sendAll(fd_,msg);
    if(!ok){
        Logger::instance().error("send failed");
        return 0;
    }
    Logger::instance().info("send success");
    return 1;
}
void TcpConnection::handleClose(){
    Logger::instance().info("client close fd="+to_string(fd_));
    connected_=0;
    //用户下线
    if(!username_.empty()){
        Logger::instance().info("user offline:"+username_);
        OnlineUserManager::instance().removeUser(username_);
        if(RedisManager::instance().connect())
          RedisManager::instance().setOffline(username_);

    }
    int oldfd=fd_;
    if(fd_!=-1){
        loop_->removeChannel(channel_);
        close(fd_);
        fd_=-1;
    }
    loop_->removeConnection(oldfd);

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
    return now-lastActiveTime_>180?1:0;
}
int TcpConnection::fd(){
    return fd_;
}
void TcpConnection::handleWrite(){
    if(outputBuffer_.size()==0)return;
    int n=::send(fd_,outputBuffer_.peek(),outputBuffer_.size(),0);
    if(n>0){
        outputBuffer_.retrieve(n);
        Logger::instance().info("send buffer bytes="+to_string(n));

        if(outputBuffer_.size()==0){
            //发送完成关闭写事件
            channel_->disableWriting();
        }
    }else{
        if(errno==EAGAIN||errno==EWOULDBLOCK)return;

        handleClose();
    }
}