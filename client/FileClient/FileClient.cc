#include "FileClient.h"
#include "../../protocol/MsgId.h"
#include "../../protocol/MessageCodec/MessageCodec.h"
#include <fstream>
#include <iostream>
#include <sys/socket.h>
#include "../../src/config.h"
#include <nlohmann/json.hpp>
#include "../../netlib/base/Logger.h"
using json=nlohmann::json;
using namespace std;

void FileClient::sendFile(int fd,const string& filename,const string& toname){
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
    char buffer[1024*4];
    int blockId=0; 
    while(1){//每次读4kb
        file.read(buffer,sizeof(buffer));
        int size=file.gcount();//gcount()：获取本次read实际读到的字节数
        if(size<=0) break;
        json js;
        js["msgid"]=FILE_DATA_MSG;
        js["toname"]=toname;
        js["filename"]=filename;
        js["blockid"]=blockId++;

        string fileData(buffer,size);
        string sendData =MessageCodec::encodeBinary(FILE_DATA_MSG,js,fileData);
        send(fd,sendData.data(),sendData.size(),0);
    }
    //发送完成,通知
    json finish;
    finish["msgid"]=FILE_FINISH_MSG;
    finish["toname"]=toname;
    finish["filename"]=filename;
    finish["fromname"]=username_;

    string sendData=MessageCodec::encode(finish.dump());
    //send(fd,finishData.data(),finishData.size(),0);

    int ret=send(fd,sendData.data(),sendData.size(),0);

cout<<"send file block:"
    <<blockId
    <<" bytes="
    <<sendData.size()
    <<" ret="
    <<ret
    <<endl;


    file.close();
    return ;
}
void FileClient::setPendingFile(const string& sender,const string& filename,long long size){
    pendingFileSender_=sender;
    pendingFilename_=filename;
    pendingFileSize_=size;
}
string FileClient::getPendingFileSender(){
    return pendingFileSender_;
}
string FileClient::getPendingFilename(){
    return pendingFilename_;
}
long long FileClient::getPendingFileSize(){
    return pendingFileSize_;
}
void FileClient::setUsername(const string& name){
    username_=name;
}