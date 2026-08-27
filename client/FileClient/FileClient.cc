#include "FileClient.h"
#include "../../protocol/MsgId.h"
#include "../../manager/FileManager/FileManager.h"
#include "../../protocol/MessageCodec/MessageCodec.h"
#include <fstream>
#include <fcntl.h>
#include <unistd.h>
#include <thread>
#include <filesystem>
#include <utility>
#include "../../netlib/base/SocketUtil/SocketUtil.h"
#include <iostream>
#include "../../utils/MD5/MD5.h"
#include <sys/socket.h>
#include "../../src/config.h"
#include <nlohmann/json.hpp>
#include "../../netlib/base/Logger/Logger.h"
#include <cerrno>
#include <system_error>
using json=nlohmann::json;
using namespace std;
#define FILE_BLOCK_SIZE (128*1024)
namespace{
string makeSendKey(const string& targetType,const string& target,const string& filename){
    return targetType+"_"+target+"_"+filename;
}
}
FileClient::FileClient(){
    stopWorkers_=false;
    receiveWorkers_.reserve(RECEIVE_WORKER_COUNT);
    for(size_t i=0;i<RECEIVE_WORKER_COUNT;i++){
        receiveWorkers_.emplace_back(&FileClient::receiveWorker,this);
    }
}
FileClient::~FileClient(){
    stopWorkers();
}
void FileClient::stopWorkers(){
    bool expected=false;
    if(!stopWorkers_.compare_exchange_strong(expected,true)){
        return;
    }
    writeCondition_.notify_all();
    for(auto& worker:receiveWorkers_){
        if(worker.joinable()){
            worker.join();
        }
    }
    receiveWorkers_.clear();
}
bool FileClient::sendPacket(int fd,const string& data){
    if(fd<0){
        return false;
    }
    lock_guard<mutex> lock(sendMutex_);
    return SocketUtil::sendAll(fd,data);
}
//发送文件->加线程
void FileClient::sendFile(int fd,int fileid,const string& filepath,const string& filename,const string& target,const string& targetType,const string& groupname,long long offset){
    string sender=username_;//防止重复发送
    {
        lock_guard<mutex> lock(mutex_);
        auto it=sendingFiles_.find(fileid);
        if(it!=sendingFiles_.end()&&it->second){
            return;
        }
        sendingFiles_[fileid]=true;
    }
    thread([this,fd,fileid,filepath,filename,target,targetType,groupname,offset,sender](){
        struct SendGuard{
            FileClient* client;
            int fileid;
            ~SendGuard(){
                lock_guard<mutex> lock(client->mutex_);
                client->sendingFiles_[fileid]=false;
            }
        } guard{this,fileid};
        int filefd=-1;
        try{
            filefd=open(filepath.c_str(),O_RDONLY);//打开文件
            if(filefd<0){
                cout<<"open file failed:"<<filepath<<endl;
                return;
            }
            error_code fileSizeError;
            long long filesize=filesystem::file_size(filepath,fileSizeError);
            if(fileSizeError){
                cout<<"get file size failed:"<<filepath<<endl;
                close(filefd);
                return;
            }
            if(offset<0||offset>filesize){
                cout<<"invalid resume offset="<<offset<<" filesize="<<filesize<<endl;
                close(filefd);
                return;
            }
            if(offset>0){
                if(lseek(filefd,offset,SEEK_SET)<0){
                    cout<<"seek file failed:"<<filepath<<endl;
                    close(filefd);
                    return;
                }
            }
            vector<char> buffer(FILE_BLOCK_SIZE);
            long long current=offset;
            bool completed=true;
            while(current<filesize){
                ssize_t n=read(filefd,buffer.data(),buffer.size());
                if(n<0){
                    if(errno==EINTR){
                        continue;
                    }
                    completed=false;
                    break;
                }
                if(n==0){
                    completed=false;
                    break;
                }
                json js;
                js["msgid"]=FILE_DATA_MSG;
                js["fileid"]=fileid;
                js["filename"]=filename;
                js["fromname"]=sender;
                js["filesize"]=filesize;
                js["offset"]=current;
                js["size"]=n;
                js["targetType"]=targetType;
                if(targetType=="user"){
                    js["toname"]=target;
                }else{
                    js["receiver"]=target;
                    js["groupname"]=groupname;
                }
                string packet=MessageCodec::encodeBinary(FILE_DATA_MSG,js,buffer.data(),n);
                if(!sendPacket(fd,packet)){
                    cout<<"send file failed:"<<filename<<endl;
                    close(filefd);
                    return;
                }
                current+=n;
            }
            close(filefd);
            filefd=-1;
            if(!completed||current!=filesize){
                cout<<"file read incomplete:"<<filename<<endl;
                return;
            }
            json finish;
            finish["msgid"]=FILE_FINISH_MSG;
            finish["fileid"]=fileid;
            finish["filename"]=filename;
            finish["fromname"]=sender;
            finish["filesize"]=filesize;
            finish["targetType"]=targetType;
            if(targetType=="user"){
                finish["toname"]=target;
            }else{
                finish["receiver"]=target;
                finish["groupname"]=groupname;
            }
            string finishData=MessageCodec::encode(finish.dump());
            if(!sendPacket(fd,finishData)){
                cout<<"send file finish failed:"<<filename<<endl;
                return;
            }
            cout<<"file send finish:"<<filename<<endl;
        }catch(const exception& e){
            if(filefd>=0){
                close(filefd);
            }
            cout<<"file transfer failed:"<<e.what()<<endl;
        }
    }).detach();
}
void FileClient::setSendFilePath(const string& targetType,const string& target,const string& filename,const string& filepath){
    lock_guard<mutex> lock(mutex_);
    sendFilePaths_[makeSendKey(targetType,target,filename)]=filepath;
}
string FileClient::getSendFilePath(const string& targetType,const string& target,const string& filename){
    lock_guard<mutex> lock(mutex_);
    auto it=sendFilePaths_.find(makeSendKey(targetType,target,filename));
    if(it==sendFilePaths_.end()){
        return "";
    }
    return it->second;
}
void FileClient::addPendingFile(const string& sender,const string& filename,long long size,const string& targetType,const string& groupname,int fileid){
    lock_guard<mutex> lock(mutex_);
    PendingFile file;
    file.filename=filename;
    file.sender=sender;
    file.filesize=size;
    file.fileid=fileid;
    file.targetType=targetType;
    file.groupname=groupname;
    pendingFiles_[sender+"_"+filename]=file;
    cout<<"add pending file success:"<<sender+"_"+filename<<endl;
}
void FileClient::setUsername(const string& name){
    lock_guard<mutex> lock(mutex_);
    username_=name;
}
string FileClient::getUsername(){
    lock_guard<mutex> lock(mutex_);
    return username_;
}
PendingFile FileClient::getPendingFile(const string& sender,const string& filename){
    lock_guard<mutex> lock(mutex_);
    string key=sender+"_"+filename;
    cout<<"search pending:"<<key<<endl;
    auto it=pendingFiles_.find(key);
    if(it!=pendingFiles_.end()){
        cout<<"found pending"<<endl;
        return it->second;
    }
    cout<<"not found pending"<<endl;
    return PendingFile{};
}
//接收文件的状态写入队列
void FileClient::receiveFile(const FilePacket& packet,int fd){
    const json& js=packet.info;
    if(!js.contains("fileid")||!js.contains("fromname")||!js.contains("filename")||!js.contains("offset")||!js.contains("filesize")){
        cout<<"接收文件数据包参数缺失"<<endl;
        return;
    }
    int fileid=-1;
    string fromname;
    string filename;
    long long offset=-1;
    long long filesize=-1;
    try{
        fileid=js["fileid"].get<int>();
        fromname=js["fromname"].get<string>();
        filename=js["filename"].get<string>();
        offset=js["offset"].get<long long>();
        filesize=js["filesize"].get<long long>();
    }catch(const exception& e){
        cout<<"接收文件数据包参数类型错误:"<<e.what()<<endl;
        return;
    }
    if(fileid<0||offset<0||filesize<0||offset>filesize||packet.data.size()>static_cast<size_t>(filesize-offset)){
        cout<<"接收文件数据包无效,fileid="<<fileid<<endl;
        return;
    }
    string username=getUsername();
    string dir=FILE_ROOT+username+"/"+to_string(fileid)+"/";
    error_code dirError;
    filesystem::create_directories(dir,dirError);
    if(dirError){
        cout<<"创建接收目录失败:"<<dir<<endl;
        return;
    }
    string filepath=dir+fromname+"_"+filename;
    string stateKey=to_string(fileid)+"_"+filepath;
    shared_ptr<ReceiveState> state;
    {
        lock_guard<mutex> lock(mutex_);
        auto it=receiveStates_.find(stateKey);
        if(it==receiveStates_.end()){
            state=make_shared<ReceiveState>();
            state->fileid=fileid;
            state->filepath=filepath;
            state->filesize=filesize;
            state->fd=fd;
            receiveStates_[stateKey]=state;
        }else{
            state=it->second;
            state->fd=fd;
        }
    }
    if(packet.data.empty()){
        return;
    }
    WriteTask task;
    task.state=state;
    task.chunk.offset=offset;
    task.chunk.data=packet.data;
    {
        lock_guard<mutex> lock(writeMutex_);
        if(stopWorkers_){
            return;
        }
        writeQueue_.emplace_back(move(task));
    }
    writeCondition_.notify_one();
}
void FileClient::receiveWorker(){
    while(true){
        WriteTask task;
        {
            unique_lock<mutex> lock(writeMutex_);
            writeCondition_.wait(lock,[this](){
                return stopWorkers_||!writeQueue_.empty();
            });
            if(stopWorkers_&&writeQueue_.empty()){
                return;
            }
            task=move(writeQueue_.front());
            writeQueue_.pop_front();
        }
        if(!task.state){
            continue;
        }
        shared_ptr<ReceiveState> state=task.state;
        int filefd=open(state->filepath.c_str(),O_WRONLY|O_CREAT,0644);
        if(filefd<0){
            cout<<"打开接收文件失败:"<<state->filepath<<endl;
            continue;
        }
        if(state->filesize>0){
            if(ftruncate(filefd,state->filesize)<0){
                cout<<"设置接收文件大小失败:"<<state->filepath<<endl;
                close(filefd);
                continue;
            }
        }
        ssize_t ret=pwrite(filefd,task.chunk.data.data(),task.chunk.data.size(),task.chunk.offset);
        close(filefd);
        if(ret!=(ssize_t)task.chunk.data.size()){
            cout<<"写入接收文件失败:"<<state->filepath<<endl;
            continue;
        }
        json ack;
        ack["msgid"]=FILE_BLOCK_ACK;
        ack["fileid"]=state->fileid;
        ack["offset"]=task.chunk.offset;
        ack["size"]=task.chunk.data.size();
        ack["filesize"]=state->filesize;
        int ackFd=state->fd.load();
        string ackData=MessageCodec::encode(ack.dump());
        if(ackFd>=0&&!sendPacket(ackFd,ackData)){
            cout<<"发送文件进度失败"<<endl;
        }
    }
}
void FileClient::connectionClosed(int fd){
    lock_guard<mutex> lock(mutex_);
    for(auto& item:receiveStates_){
        if(item.second->fd.load()==fd){
            item.second->fd.store(-1);
        }
    }
}