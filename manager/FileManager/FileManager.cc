#include "FileManager.h"
#include "../../netlib/base/Logger.h"
#include <iostream>
#include <algorithm>
using namespace std;

void FileManager::startReceive(
    int fileid,
    const string& fromname,
    const string& filename,
    long long filesize,bool resume){

    auto it=files_.find(fileid);


    //断点续传
    if(it!=files_.end())
        return;



    ReceiveFileInfo info;


    info.fileid=fileid;

    info.fromname=fromname;

    info.filename=filename;

    info.filesize=filesize;

    info.receivedSize=0;

    info.lastBlock=-1;

    if(resume){
        // 断点续传：
        // 这里不能假设 receivedSize 从 0 开始
        info.receivedSize = 0;
        info.lastBlock = -1;

        cout << "resume receive:"
             << filename
             << " size="
             << filesize
             << endl;
    }
    else{
        info.receivedSize = 0;
        info.lastBlock = -1;

        cout << "start receive:"
             << filename
             << " size="
             << filesize
             << endl;
    }


    files_[fileid]=info;



    cout<<"start receive:"
        <<filename
        <<" size="
        <<filesize
        <<endl;

}

bool FileManager::updateBlock(int fileid,int blockid,int size){

    auto it=files_.find(fileid);


    if(it==files_.end()){

        cout<<"file not found"<<endl;

        Logger::instance()
        .error("file not found");

        return false;
    }



    ReceiveFileInfo& info=it->second;



    auto blockIt=find(
        info.receivedBlocks.begin(),
        info.receivedBlocks.end(),
        blockid
    );



    if(blockIt!=info.receivedBlocks.end()){

        cout<<"duplicate block:"
            <<blockid
            <<endl;

        return false;
    }



    info.receivedBlocks.push_back(blockid);



    info.receivedSize+=size;


    info.lastBlock=blockid;



    cout<<"recv:"
        <<info.receivedSize
        <<"/"
        <<info.filesize
        <<" block="
        <<blockid
        <<endl;



    return true;

}

bool FileManager::checkFinish(int fileid){

    auto it=files_.find(fileid);


    if(it==files_.end())
        return false;



    return it->second.receivedSize
        ==
        it->second.filesize;

}

bool FileManager::exists(int fileid){

    return files_.find(fileid)
        !=files_.end();

}
void FileManager::resumeReceive(
    int fileid,
    const string& fromname,
    const string& filename,
    long long filesize,
    const vector<int>& receivedBlocks)
{
    auto it = files_.find(fileid);

    // 如果已经存在，说明当前客户端进程
    // 本来就保存着这个文件状态
    if(it != files_.end())
    {
        cout << "resume file already exists: "
             << filename
             << endl;

        return;
    }

    ReceiveFileInfo info;

    info.fileid = fileid;

    info.fromname = fromname;

    info.filename = filename;

    info.filesize = filesize;

    info.lastBlock = -1;

    // 保存已经收到的 block
    info.receivedBlocks = receivedBlocks;

    /*
     * 重新计算已经收到多少字节
     *
     * 不能简单：
     *
     * receivedBlocks.size() * 4096
     *
     * 因为最后一个 block 可能不足4096。
     */
    const long long BLOCK_SIZE = 4096;

    for(int blockid : receivedBlocks)
    {
        long long offset =
            static_cast<long long>(blockid) * BLOCK_SIZE;

        if(offset >= filesize)
            continue;

        long long blockSize =
            std::min(
                BLOCK_SIZE,
                filesize - offset
            );

        info.receivedSize += blockSize;
    }

    if(!receivedBlocks.empty())
    {
        info.lastBlock =
            receivedBlocks.back();
    }

    files_[fileid] = info;

    cout << "resume receive:"
         << filename
         << " size="
         << filesize
         << endl;

    cout << "already received:"
         << receivedBlocks.size()
         << " blocks"
         << endl;

    cout << "already received bytes:"
         << info.receivedSize
         << endl;
}
bool FileManager::hasBlock(
    int fileid,
    int blockid)
{
    auto it = files_.find(fileid);

    if(it == files_.end())
        return false;

    ReceiveFileInfo& info = it->second;

    for(int block : info.receivedBlocks)
    {
        if(block == blockid)
            return true;
    }

    return false;
}