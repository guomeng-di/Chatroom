#include "FileManager.h"
#include "../../netlib/base/Logger/Logger.h"
#include <iostream>
using namespace std;

void FileManager::startReceive(int fileid,const string& fromname,const string& filename,long long filesize,long long receivedSize){
    if(files_.find(fileid)!=files_.end())
        return;

    ReceiveFileInfo info;
    info.fileid=fileid;
    info.fromname=fromname;
    info.filename=filename;
    info.filesize=filesize;
    //断点位置
    info.receivedSize=receivedSize;
    //新增
    info.updateCount=0;
    files_[fileid]=info;
    LOG_INFO<<"开始接收文件:"<<filename<<" 当前偏移位置="<<receivedSize;
}
bool FileManager::updateSize(int fileid,long long size)
{
    auto it=files_.find(fileid);

    if(it==files_.end())
        return false;


    it->second.receivedSize=size;


    it->second.updateCount++;


    LOG_INFO<<"文件ID:"
            <<fileid
            <<" 已接收大小:"
            <<size
            <<" 字节";


    if(it->second.updateCount>=20)
    {
        it->second.updateCount=0;

        return true;
    }


    return false;
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