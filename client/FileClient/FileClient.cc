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
class TcpConnection;
using json=nlohmann::json;
using namespace std;
#define FILE_BLOCK_SIZE (128*1024)
namespace {
string makeSendKey(const string& targetType,const string& target,const string& filename){
    return targetType + "_" + target + "_" + filename;
}
}
//发送方->服务器
void FileClient::sendFile(int fd,int fileid,const string& filepath,const string& filename,const string& target,const string& targetType,const string& groupname,long long offset){
    string sender=username_;
    {
        lock_guard<mutex> lock(mutex_);
        if(sendingFiles_[fileid]) return;
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
      try{
        int filefd =open(filepath.c_str(),O_RDONLY);
        if(filefd < 0){
            cout<<"open file failed:"<<filepath<<endl;
            return;

        }

        error_code fileSizeError;
        long long filesize = filesystem::file_size(filepath,fileSizeError);
        if(fileSizeError){
            cout<<"get file size failed:"<<filepath<<endl;
            close(filefd);
            return;
        }

        if(offset < 0 || offset > filesize){
            cout<<"invalid resume offset="<<offset<<" filesize="<<filesize<<endl;
            close(filefd);
            return;
        }

        if(offset> 0){
            //cout<<"resume send offset="<<offset<<endl;

            if(lseek(filefd,offset,SEEK_SET) < 0){
                cout<<"seek file failed:"<<filepath<<endl;
                close(filefd);
                return;
            }
        }

        vector<char> buffer(FILE_BLOCK_SIZE);
        long long current=offset;
        bool completed=true;
        while(current<filesize){
            int n =read(filefd,buffer.data(),sizeof(buffer));
            if(n<=0){
                completed=false;
                break;
            }
            json js;
            js["msgid"]=FILE_DATA_MSG;
            js["fileid"]=fileid;
            js["filename"]=filename;
            js["fromname"] =sender;
            js["filesize"]=filesize;
            js["offset"] =current;
            js["size"]=n;
            js["targetType"]=targetType;
            if(targetType=="user")js["toname"]=target;
            else{
                js["receiver"]=target;
                js["groupname"] =groupname;
            }
           string packet =
MessageCodec::encodeBinary(
    FILE_DATA_MSG,
    js,
    buffer.data(),
    n
);
if(!SocketUtil::sendAll(fd,packet))
{
    cout<<"send file failed"<<endl;
    close(filefd);
    return;
}
            current+= n;
        }
        close(filefd);

        if(!completed || current != filesize){
            cout<<"file read incomplete:"<<filename<<endl;
            return;
        }

        json finish;
        finish["msgid"] =FILE_FINISH_MSG;
        finish["fileid"]=fileid;
        finish["filename"] =filename;
        finish["fromname"]=sender;
        finish["filesize"]=filesize;
        finish["targetType"]=targetType;
        if(targetType=="user")finish["toname"]=target;
        else{
            finish["receiver"]=target;
            finish["groupname"]=groupname;
        }
        string finishData =MessageCodec::encode(finish.dump());
        // conn->sendBinary(std::move(finishData));
        SocketUtil::sendAll(fd,finishData);
        cout<<"file send finish:"<<filename<<endl;
      }catch(const exception& e){
        cout<<"file transfer failed:"<<e.what()<<endl;
      }
    }).detach();
}
void FileClient::setSendFilePath(const string& targetType,const string& target,const string& filename,const string& filepath){
    lock_guard<mutex> lock(mutex_);
    sendFilePaths_[makeSendKey(targetType,target,filename)] = filepath;
}
string FileClient::getSendFilePath(const string& targetType,const string& target,const string& filename){
    lock_guard<mutex> lock(mutex_);
    auto it = sendFilePaths_.find(makeSendKey(targetType,target,filename));
    if(it == sendFilePaths_.end()) return "";
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
    username_=name;
}
string FileClient::getUsername(){
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

void FileClient::receiveFile(const FilePacket& packet,int fd){
    json js=packet.info;
    if(!js.contains("fileid") || !js.contains("fromname") ||
       !js.contains("filename") || !js.contains("offset") ||
       !js.contains("filesize")){
        cout<<"接收文件数据包参数缺失"<<endl;
        return;
    }

    int fileid=js["fileid"];
    string fromname=js["fromname"];
    string filename=js["filename"];
    long long offset=js["offset"];
    long long filesize=js["filesize"];

    if(fileid<0 || offset<0 || filesize<0 ||
       offset>filesize || packet.data.size()>static_cast<size_t>(filesize-offset)){
        cout<<"接收文件数据包无效,fileid="<<fileid<<endl;
        return;
    }

    string username=getUsername();
    string dir=FILE_ROOT+username+"/"+to_string(fileid)+"/";
    filesystem::create_directories(dir);
    string filepath=dir+fromname+"_"+filename;
    string stateKey=to_string(fileid)+"_"+filepath;

    shared_ptr<ReceiveState> state;
    bool startWorker=false;
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
        if(!state->workerStarted){
            state->workerStarted=true;
            startWorker=true;
        }
    }

    if(startWorker){
        thread([state](){
            int filefd=open(state->filepath.c_str(),O_WRONLY|O_CREAT,0644);
            if(filefd<0){
                cout<<"打开接收文件失败:"<<state->filepath<<endl;
                return;
            }
            while(true){
                ReceiveChunk chunk;
                {
                    unique_lock<mutex> lock(state->mutex);
                    state->condition.wait(lock,[state](){
                        return !state->chunks.empty();
                    });
                    chunk=move(state->chunks.front());
                    state->queuedBytes-=chunk.data.size();
                    state->chunks.pop_front();
                }
                state->condition.notify_all();
                ssize_t ret=pwrite(filefd,chunk.data.data(),chunk.data.size(),chunk.offset);
                if(ret!=(ssize_t)chunk.data.size()){
                    cout<<"写入接收文件失败:"<<state->filepath<<endl;
                    continue;
                }
                json ack;
                ack["msgid"]=FILE_BLOCK_ACK;
                ack["fileid"]=state->fileid;
                ack["offset"]=chunk.offset;
                ack["size"]=chunk.data.size();
                ack["filesize"]=state->filesize;
                string ackData=MessageCodec::encode(ack.dump());
                int ackFd=state->fd;
                if(ackFd>=0 && !SocketUtil::sendAll(ackFd,ackData)){
                    cout<<"发送文件进度失败"<<endl;
                }
            }
            close(filefd);
        }).detach();
    }

    if(packet.data.empty()) return;
    {
        lock_guard<mutex> lock(state->mutex);
        state->chunks.push_back(ReceiveChunk{offset,packet.data});
        state->queuedBytes+=packet.data.size();
    }
    state->condition.notify_one();
}
void FileClient::connectionClosed(int fd){
    lock_guard<mutex> lock(mutex_);
    for(auto& item:receiveStates_){
        if(item.second->fd==fd) item.second->fd=-1;
    }
}
