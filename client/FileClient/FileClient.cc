#include "FileClient.h"
#include "../../protocol/MsgId.h"
#include "../../manager/FileManager/FileManager.h"
#include "../../protocol/MessageCodec/MessageCodec.h"
#include <fstream>
#include <fcntl.h>
#include <unistd.h>
#include <filesystem>
#include "../../netlib/base/SocketUtil/SocketUtil.h"
#include <iostream>
#include "../../utils/MD5/MD5.h"
#include <sys/socket.h>
#include "../../src/config.h"
#include <nlohmann/json.hpp>
#include "../../netlib/base/Logger.h"
#include "../../model/FileModel/FileModel.h"
using json=nlohmann::json;
using namespace std;
#define FILE_BLOCK_SIZE 4096
//发送方->服务器
void FileClient::sendFile(int fd,int fileid,const string& filename,const std::string& target,const string& targetType,const string& groupname,vector<int> receivedBlocks){
    //拼接完整路径
    string filepath=FILE_ROOT+filename;
    cout<<"尝试打开发送文件："<<filepath<<endl;
    //从磁盘读取文件到程序内存
    ifstream file(filepath,ios::binary);
    if(!file.is_open()){
        Logger::instance().error("open file failed");
        cout<<"open file failed";
        return ;
    }
    // 文件总大小
    long long filesize= filesystem::file_size(filepath);

    // 打印断点续传信息
     cout << "==========发送文件==========" << endl;
    cout << "fileid=" << fileid << endl;
    cout << "filename=" << filename << endl;
    cout << "target=" << target << endl;
    cout << "targetType=" << targetType << endl;
    cout << "filesize=" << filesize << endl;

    cout << "服务器已经收到 "
         << receivedBlocks.size()
         << " 个block" << endl;

    if(!receivedBlocks.empty()){
        cout << "已经收到的block：";
        for(int block : receivedBlocks) cout << block << " ";
        
        cout << endl;
    }
    cout << "============================" << endl;

    char buffer[FILE_BLOCK_SIZE];
    int blockId=0; 
    while(1){//每次读4kb
        file.read(buffer,sizeof(buffer));
        int size=file.gcount();//gcount()：获取本次read实际读到的字节数
        
        cout<<"read block:"<<blockId<<" size="<<size<<" eof="<<file.eof()<<" pos="<<file.tellg()<<endl;
        if(size<=0) break;
        if(existBlock(blockId,receivedBlocks)){
            cout<<"skip block:"<<blockId<<" current stream pos="<<file.tellg()<<endl;
            blockId++;
            continue;
        }
        json js;
        js["msgid"]=FILE_DATA_MSG;
        js["fromname"]=username_;
        //js["toname"]=toname;
        js["fileid"]=fileid;
        js["filename"]=filename;
        js["blockid"]=blockId;
        js["filesize"]=filesize;
        js["targetType"]=targetType;
        if(targetType=="user") js["toname"]=target;
        else if(targetType=="group"){
            js["groupname"]=groupname;
            js["receiver"]=target;
        }


        string fileData(buffer, size);
        string sendData =MessageCodec::encodeBinary(FILE_DATA_MSG,js,fileData);
        cout<<"prepare send block:"<<blockId<<" size="<<size<<endl;
        if(!SocketUtil::sendAll(fd, sendData)){
            Logger::instance().error( "send file block failed");
            //file.close();
            return;
        }
        cout << "send block:"<< blockId<< " size="<< size<< endl;
        // 等待这个 block 的 ACK
        if(!waitForBlockAck(fileid, blockId)){
            Logger::instance().error("block ack timeout, stop sending");
            cout<<"文件发送中断:"<<" block="<<blockId<< endl;
            return;
    }
        // 这里只增加一次
        blockId++;
    }
    file.close();
    //发送完成,通知
    json finish;
    finish["msgid"]=FILE_FINISH_MSG;
    string md5=MD5::getFileMD5(filepath);
    finish["md5"]=md5;
    //finish["toname"]=toname;
    finish["filename"]=filename;
    finish["fromname"]=username_;
    finish["filesize"]=filesize;
    finish["fileid"]=fileid;
    finish["targetType"]=targetType;
    if(targetType=="user") finish["toname"]=target;
    else if(targetType=="group"){
        finish["receiver"]=target;
        finish["groupname"]=groupname;
    }

    string sendData=MessageCodec::encode(finish.dump());
    //send(fd,finishData.data(),finishData.size(),0);

    bool ret=SocketUtil::sendAll(fd,sendData);
    if(ret) cout<<"send finish packet"<<filename<<endl;


}
void FileClient::addPendingFile(const string& sender,const string& filename,long long size,const string& targetType,const string& groupname){
    PendingFile file;
    file.filename=filename;
    file.sender=sender;
    file.filesize=size;
    file.targetType=targetType;
    file.groupname=groupname;

    pendingFiles_[sender+"_"+filename]=file;

    cout<<"add pending file:"
        <<endl;

}
void FileClient::setUsername(const string& name){
    username_=name;
}
string FileClient::getUsername(){
    return username_;
}
PendingFile FileClient::getPendingFile(const string& sender,const string& filename){
    string key=sender+"_"+filename;
    
     cout<<"search pending:"
        <<key<<endl;

    auto it=pendingFiles_.find(key);
    if(it!=pendingFiles_.end()){
        
        cout<<"found pending"<<endl;

        return it->second;
    }

     cout<<"not found pending"<<endl;

    return PendingFile{};
}

void FileClient::receiveFile(const FilePacket& packet,int fd){
    json js = packet.info;

    string fromname = js["fromname"];
    string filename = js["filename"];
    int blockid = js["blockid"];
    int fileid = js["fileid"];
    long long filesize=js["filesize"];
    string data = packet.data;
    string username = getUsername();

    // 1. 文件保存路径
    string dir =FILE_ROOT+ username+ "/"+ to_string(fileid)+ "/";
    filesystem::create_directories(dir);
    string path =dir+ fromname+ "_"+ filename;
    bool fileExists =filesystem::exists(path);

    // 2. 初始化 FileManager
    if(!FileManager::instance().exists(fileid)){
        bool resume = fileExists;
        FileManager::instance().startReceive(fileid,fromname,filename,filesize,resume);
    }
    // 3. 先判断 block 是否重复
    if(FileManager::instance().hasBlock(fileid,blockid)){
        cout << "duplicate block ignored:"<< blockid << endl;
        return;
    }
    // 4. 打开文件
    int flags = O_WRONLY | O_CREAT;
    if(!fileExists) flags |= O_TRUNC;
    int filefd =open(path.c_str(),flags,0644);
    if(filefd < 0){
        Logger::instance().error("file open failed: "+ string(strerror(errno)));
        return;
    }
    // 5. 定位 block
    off_t offset =static_cast<off_t>(blockid)* FILE_BLOCK_SIZE;
    ssize_t written =pwrite(filefd,data.data(),data.size(),offset);
    if(written !=static_cast<ssize_t>(data.size())){
        Logger::instance().error("pwrite failed: "+ string(strerror(errno)));
        close(filefd);
        return;
    }

    close(filefd);
    cout << "receive block:"<< blockid<< " size="<< data.size()<< endl;

    // 6. 文件真正写成功
    bool recorded =FileManager::instance().updateBlock(fileid,blockid,data.size());
    if(!recorded){
        cout << "block already recorded:"<< blockid<< endl;
        return;
    }
    // pwrite 成功
    // FileManager记录成功
    json ack;
    ack["msgid"] = FILE_BLOCK_ACK;
    ack["fileid"] = fileid;
    ack["filename"] = filename;
    ack["blockid"] = blockid;
    ack["receiver"] = username;
    ack["fromname"] = fromname;

    string ackData =MessageCodec::encode(ack.dump());

    if(!SocketUtil::sendAll(fd,ackData)){
        Logger::instance().error(
            "send file block ack failed:"+ to_string(blockid));
        return;
    }

    cout << "send block ACK:"<< blockid<< endl;
}
bool FileClient::existBlock(int blockid,const std::vector<int>& blocks){
    for(auto b : blocks)
        if(b == blockid) return true;
        
    return false;
}
bool FileClient::waitForBlockAck(int fileid, int blockid){
    unique_lock<mutex> lock(ackMutex_);
    cout << "等待 ACK:"<< " fileid=" << fileid<< " blockid=" << blockid<< endl;
    // 最多等待30秒
    bool received = ackCv_.wait_for(lock,chrono::seconds(10),[&](){
            return receivedAcks_.count({fileid, blockid}) > 0;});

    // 5秒没有收到
    if(!received){
        cout << "等待 ACK 超时:"<< " fileid=" << fileid<< " blockid=" << blockid<< endl;
        return false;
    }
    // 收到ACK
    receivedAcks_.erase({fileid, blockid});
    cout << "ACK确认:"<< " fileid=" << fileid<< " blockid=" << blockid<< endl;

    return true;
}
void FileClient::notifyBlockAck(int fileid,int blockid){
    {
        std::lock_guard<std::mutex> lock(ackMutex_);
        receivedAcks_.insert({fileid,blockid});
    }
    ackCv_.notify_all();
    cout<<"收到ACK:"<<" fileid="<<fileid<<" blockid="<<blockid<<endl;
}