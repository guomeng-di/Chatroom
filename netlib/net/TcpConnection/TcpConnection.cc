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
            updateActiveTime();
            Logger::instance().info("recv bytes="+to_string(n));
            buffer_.append(buf,n);
            while(buffer_.hasMessage()){
                string msg=buffer_.retrieveMessage();
                int msgid=MessageCodec::getMsgId(msg);
                cout<<"========== SERVER RECEIVE MESSAGE =========="<<endl;
                cout<<"fd="<<fd_<<endl;
                cout<<"username="<<username_<<endl;
                cout<<"msgid="<<msgid<<endl;
                cout<<"message size="<<msg.size()<<endl;
                if(msgid==FILE_DATA_MSG){
                    cout<<"message type=FILE_DATA_MSG"<<endl;
                }else{
                    try{
                        json debugJson=JsonProtocol::decode(msg);
                        cout<<"json="<<debugJson.dump(4)<<endl;
                    }catch(const exception& e){
                        cout<<"json decode debug failed: "<<e.what()<<endl;
                    }
                }
                cout<<"============================================"<<endl;
                try{
                    if(msgid==FILE_DATA_MSG){
                        FilePacket packet=MessageCodec::decodeBinary(msg);
                        cout<<"========== FILE DATA =========="<<endl;
                        cout<<"fd="<<fd_<<endl;
                        cout<<"username="<<username_<<endl;
                        cout<<"file msgid="<<msgid<<endl;
                        cout<<"================================"<<endl;
                        FileService::receiveFileData(packet,this);
                    }else{
                        json js=JsonProtocol::decode(msg);
                        MessageDispatcher::dispatch(js,this);
                    }
                }catch(const exception& e){
                    cout<<"========== MESSAGE EXCEPTION =========="<<endl;
                    cout<<"fd="<<fd_<<endl;
                    cout<<"username="<<username_<<endl;
                    cout<<"msgid="<<msgid<<endl;
                    cout<<"message size="<<msg.size()<<endl;
                    cout<<"error="<<e.what()<<endl;
                    cout<<"======================================="<<endl;
                    handleClose();
                    return;
                }
            }
            if(buffer_.hasError()){
                cout<<"========== BUFFER ERROR =========="<<endl;
                cout<<"fd="<<fd_<<endl;
                cout<<"username="<<username_<<endl;
                cout<<"=================================="<<endl;
                handleClose();
                return;
            }
        }else if(n==0){
            cout<<"========== CLIENT CLOSED SOCKET =========="<<endl;
            cout<<"fd="<<fd_<<endl;
            cout<<"username="<<username_<<endl;
            cout<<"recv returned 0"<<endl;
            cout<<"=========================================="<<endl;
            handleClose();
            return;
        }else{
            if(errno==EAGAIN || errno==EWOULDBLOCK){
                break;
            }
            if(errno==EINTR){
                continue;
            }
            cout<<"========== RECV ERROR =========="<<endl;
            cout<<"fd="<<fd_<<endl;
            cout<<"username="<<username_<<endl;
            cout<<"errno="<<errno<<endl;
            cout<<"error="<<strerror(errno)<<endl;
            cout<<"==============================="<<endl;
            handleClose();
            return;
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
    // 二进制文件包也进入连接所属 EventLoop 的发送缓冲，避免和普通消息
    // 直接并发写 socket 导致协议包交错。
    loop_->queueInLoop(
        [this,msg](){
            outputBuffer_.append(msg.data(),msg.size());
            channel_->enableWriting();
        }
    );
    return true;
}
void TcpConnection::handleClose(){
    cout<<"========== TcpConnection::handleClose =========="<<endl;
    cout<<"fd="<<fd_<<endl;
    cout<<"username="<<username_<<endl;
    cout<<"connected="<<connected_<<endl;

    if(!connected_){
        cout<<"[CLOSE] already closed, return"<<endl;
        cout<<"================================================="<<endl;
        return;
    }

    connected_=false;

    if(!username_.empty()){
        cout<<"[CLOSE] remove online user: "<<username_<<endl;

        bool removed = OnlineUserManager::instance().removeUser(username_, this);

        cout<<"[CLOSE] set redis offline: "<<username_<<endl;

        if(removed && RedisManager::instance().connect()){
            RedisManager::instance().setOffline(username_);
        }
    }

    int oldfd=fd_;

    if(fd_!=-1){
        cout<<"[CLOSE] remove channel fd="<<fd_<<endl;

        loop_->removeChannel(channel_);

        close(fd_);

        fd_=-1;
    }

    loop_->deleteConnection(oldfd);

    cout<<"[CLOSE] handleClose finished"<<endl;
    cout<<"================================================="<<endl;
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
    if(errno==EAGAIN||errno==EWOULDBLOCK){
        return;
    }

    cout<<"========== SEND ERROR =========="<<endl;
    cout<<"fd="<<fd_<<endl;
    cout<<"username="<<username_<<endl;
    cout<<"errno="<<errno<<endl;
    cout<<"error="<<strerror(errno)<<endl;
    cout<<"==============================="<<endl;

    handleClose();
}
}
