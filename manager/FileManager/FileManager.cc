#include "FileManager.h"
#include "../../netlib/base/Logger/Logger.h"
#include <iostream>
using namespace std;

void FileManager::startReceive(int fileid,const string& fromname,const string& filename,long long filesize,long long receivedSize){
    if(files_.find(fileid)!=files_.end()) return;
    ReceiveFileInfo info;
    info.fileid=fileid;
    info.fromname=fromname;
    info.filename=filename;
    info.filesize=filesize;
    //断点位置
    info.receivedSize=receivedSize;

    files_[fileid]=info;

    LOG_INFO<<"开始接收文件:"<<filename<<" 当前偏移位置="<<receivedSize;
}
bool FileManager::updateSize(int fileid,long long size){
    auto it=files_.find(fileid);
    if(it==files_.end()) return 0;

    it->second.receivedSize=size;

    LOG_INFO<<"文件ID:"<<fileid<<" 已接收大小:"<<size<<" 字节";

    return 1;

}
long long FileManager::getReceivedSize(int fileid){
    auto it=files_.find(fileid);
    if(it==files_.end()) return 0;
    return it->second.receivedSize;
}

bool FileManager::exists(int fileid){
    return files_.find(fileid)!=files_.end();
}

bool FileManager::checkFinish(int fileid){
    auto it=files_.find(fileid);
    if(it==files_.end())return false;

    return it->second.receivedSize==it->second.filesize;
}