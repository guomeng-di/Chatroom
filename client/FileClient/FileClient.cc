#include "FileClient.h"
#include "../../protocol/MsgId.h"
#include "../../manager/FileManager/FileManager.h"
#include "../../protocol/MessageCodec/MessageCodec.h"
#include <fstream>
#include <fcntl.h>
#include <unistd.h>
#include <thread>
#include <filesystem>
#include "../../netlib/base/SocketUtil/SocketUtil.h"
#include <iostream>
#include "../../utils/MD5/MD5.h"
#include <sys/socket.h>
#include "../../src/config.h"
#include <nlohmann/json.hpp>
#include "../../netlib/base/Logger/Logger.h"
#include "../../model/FileModel/FileModel.h"
using json=nlohmann::json;
using namespace std;
#define FILE_BLOCK_SIZE 4096
namespace {
string makeSendKey(const string& targetType,const string& target,const string& filename){
    return targetType + "_" + target + "_" + filename;
}
}
//发送方->服务器
void FileClient::sendFile(int fd,int fileid,const string& filepath,const string& filename,const string& target,const string& targetType,const string& groupname,long long offset){
    // File transfer is deliberately detached from the receive/menu threads.
    // The caller returns immediately, so the user can continue chatting while
    // this worker reads and sends file packets in the background.
    thread([=](){
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
            cout<<"resume send offset="<<offset<<endl;

            if(lseek(filefd,offset,SEEK_SET) < 0){
                cout<<"seek file failed:"<<filepath<<endl;
                close(filefd);
                return;
            }
        }

        char buffer[4096];
        long long current=offset;
        bool completed=true;
        while(current<filesize){
            int n =read(filefd,buffer,sizeof(buffer));
            if(n<=0){
                completed=false;
                break;
            }
            json js;
            js["msgid"]=FILE_DATA_MSG;
            js["fileid"]=fileid;
            js["filename"]=filename;
            js["fromname"] =username_;
            js["filesize"]=filesize;
            js["offset"] =current;
            js["size"]=n;
            js["targetType"]=targetType;
            if(targetType=="user")js["toname"]=target;
            else{
                js["receiver"]=target;
                js["groupname"] =groupname;
            }
            string data(buffer,n);
            string packet =MessageCodec::encodeBinary(FILE_DATA_MSG,js,data);
            if(!SocketUtil::sendAll(fd,packet)){
                cout<<"send file failed"<<endl;
                close(filefd);
                return;
            }
            cout<<"send offset="<<current<<" size="<<n<<endl;
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
        finish["fromname"]=username_;
        finish["filesize"]=filesize;
        finish["targetType"]=targetType;
        if(targetType=="user")finish["toname"]=target;
        else{
            finish["receiver"]=target;
            finish["groupname"]=groupname;
        }
        string finishData =MessageCodec::encode(finish.dump());
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

    int fileid=js["fileid"];
    string fromname=js["fromname"];
    string filename=js["filename"];
    long long filesize=js["filesize"];
    long long offset=js["offset"];

    string data=packet.data;
    string username=getUsername();
    string dir=FILE_ROOT+username+"/"+to_string(fileid)+"/";
    filesystem::create_directories(dir);
    string filepath=dir+fromname+"_"+filename;

    int filefd=open(filepath.c_str(),O_WRONLY|O_CREAT,0644);
    if(filefd<0){
        cout<<"open receive file failed"<<endl;
        return;
    }
    ssize_t ret=pwrite(filefd,data.data(),data.size(),offset);
    close(filefd);
    if(ret!=(ssize_t)data.size()){
        cout<<"pwrite failed"<<endl;
        return;
    }

    cout<<"receive offset="<<offset<<" size="<<data.size()<<endl;
    FileModel model;
    long long oldSize=model.getReceivedSize(fileid,username_);
    long long newSize=offset + data.size();
    if(newSize < oldSize) newSize = oldSize;
    model.updateReceivedSize(fileid,username,newSize);
    cout<<"========== RECEIVE UPDATE =========="<<endl;
    cout<<"fileid="<<fileid<<endl;
    cout<<"received="<<newSize<<"/"<<filesize<<endl;
    cout<<"===================================="<<endl;
}
