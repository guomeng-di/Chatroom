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
#include "../../base/Logger/Logger.h"
using namespace std;
TcpConnection::TcpConnection(EventLoop* loop,int fd):
fd_(fd),loop_(loop),channel_(new Channel(loop_,fd_)),connected_(true){
    lastActiveTime_=time(nullptr);

    LOG_INFO<<"创建TcpConnection成功 fd="<<fd_;
    channel_->setReadCallback(std::bind(&TcpConnection::handleRead,this));
    channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite,this));
    channel_->setCloseCallback(std::bind(&TcpConnection::handleClose,this));
    LOG_INFO<<"设置TcpConnection回调成功 fd="<<fd_;
    channel_->enableReading();
    LOG_INFO<<"客户端读事件监听开启 fd="<<fd_;
}
TcpConnection::~TcpConnection(){
    // 释放channel
    LOG_INFO<<"销毁TcpConnection fd="<<fd_;
    if(channel_){
        delete channel_;
        channel_=nullptr;}
    
}
void TcpConnection::handleRead(){

    LOG_INFO<<"开始读取客户端数据 fd="<<fd_;

    char buf[1024*1024];

    while(1){

        int n=recv(fd_,buf,sizeof(buf),0);

        if(n>0){

            updateActiveTime();

            //LOG_INFO<<"收到客户端数据 fd="<<fd_<<" bytes="<<n;

            buffer_.append(buf,n);

            while(buffer_.hasMessage()){

                string msg=buffer_.retrieveMessage();

                int msgid=MessageCodec::getMsgId(msg);

                if(msgid!=FILE_DATA_MSG){

                    // LOG_INFO<<"解析消息 fd="<<fd_
                    //         <<" username="<<username_
                    //         <<" msgid="<<msgid
                    //         <<" size="<<msg.size();

                }

                try{

                    if(msgid==FILE_DATA_MSG){

                        FilePacket packet=MessageCodec::decodeBinary(msg);

                        FileService::receiveFileData(packet,this);

                    }else{

                        json js=JsonProtocol::decode(msg);

                        LOG_INFO<<"消息JSON解析成功 fd="<<fd_
                                <<" msgid="<<msgid;

                        MessageDispatcher::dispatch(js,this);

                    }

                }catch(const exception& e){

                    LOG_ERROR<<"消息处理异常 fd="<<fd_
                             <<" msgid="<<msgid
                             <<" error="<<e.what();

                    handleClose();

                    return;

                }

            }

            if(buffer_.hasError()){

                LOG_ERROR<<"Buffer解析异常 fd="<<fd_;

                handleClose();

                return;

            }

        }else if(n==0){

            LOG_WARN<<"客户端关闭连接 fd="<<fd_
                    <<" username="<<username_;

            handleClose();

            return;

        }else{

            if(errno==EAGAIN || errno==EWOULDBLOCK){

                break;

            }

            if(errno==EINTR){

                continue;

            }

            LOG_ERROR<<"recv读取失败 fd="<<fd_
                     <<" errno="<<errno
                     <<" error="<<strerror(errno);

            handleClose();

            return;

        }

    }

}
void TcpConnection::send(const string& msg){
    LOG_INFO<<"准备发送普通消息 fd="<<fd_<<" size="<<msg.size();
    loop_->queueInLoop(
        [this,msg](){
            string data=MessageCodec::encode(msg);
            outputBuffer_.append(data.data(),data.size());
            LOG_INFO<<"消息加入发送缓冲区 fd="<<fd_<<" size="<<data.size();
            channel_->enableWriting();
            handleWrite();
        }
    );
}
bool TcpConnection::sendBinary(const string& msg){
    LOG_INFO<<"准备发送二进制数据 fd="<<fd_<<" size="<<msg.size();
    // 二进制文件包也进入连接所属 EventLoop 的发送缓冲，避免和普通消息直接并发写 socket 导致协议包交错。
    loop_->queueInLoop(
        [this,msg](){
            outputBuffer_.append(msg.data(),msg.size());
            LOG_INFO<<"二进制数据加入发送缓冲区 fd="<<fd_<<" size="<<msg.size();
            channel_->enableWriting();
            handleWrite();
        }
    );
    return true;
}
void TcpConnection::handleClose(){
     //LOG_INFO<<"开始关闭TcpConnection fd="<<fd_<<" username="<<username_;
    if(!connected_){
        LOG_WARN<<"连接已经关闭,重复关闭 fd="<<fd_;
        return;
    }

    connected_=false;

    if(!username_.empty()){
        LOG_INFO<<"移除在线用户 username="<<username_;
        bool removed = OnlineUserManager::instance().removeUser(username_, this);
        if(removed && RedisManager::instance().connect()){
            LOG_INFO<<"Redis设置用户离线 username="<<username_;
            RedisManager::instance().setOffline(username_);
        }
    }

    int oldfd=fd_;

    if(channel_){
        loop_->removeChannel(channel_);
    }

    if(fd_!=-1){
        // LOG_INFO<<"移除Channel fd="<<fd_;
        // loop_->removeChannel(channel_);

        close(fd_);
        fd_=-1;
    }
    loop_->deleteConnection(oldfd);
    //LOG_INFO<<"TcpConnection关闭完成 fd="<<oldfd;
}
void TcpConnection::setUsername(const string& username){
    username_=username;
    LOG_INFO<<"绑定用户到连接 username="<<username_<<" fd="<<fd_;
}
string TcpConnection::getUsername(){
    return username_;
}
void TcpConnection::updateActiveTime(){
    lastActiveTime_=time(nullptr);
}
bool TcpConnection::isTimeout(){
    time_t now=time(nullptr);
    bool timeout=(now-lastActiveTime_>180);
    if(timeout){
        LOG_WARN<<"检测到连接超时 fd="<<fd_<<" username="<<username_;
    }
    return timeout;
}
int TcpConnection::fd(){
    return fd_;
}
void TcpConnection::handleWrite(){

    while(outputBuffer_.size()>0){

        ssize_t n = ::send(
            fd_,
            outputBuffer_.peek(),
            outputBuffer_.size(),
            MSG_NOSIGNAL
        );

        if(n>0){

            outputBuffer_.retrieve(n);

        }else{

            if(errno==EAGAIN || errno==EWOULDBLOCK){
                break;
            }

            if(errno==EINTR){
                continue;
            }

            LOG_ERROR<<"send error "
                     <<strerror(errno);

            handleClose();
            return;
        }
    }


    if(outputBuffer_.size()==0){
        channel_->disableWriting();
    }

}
