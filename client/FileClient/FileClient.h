#pragma once
#include <string>
#include <map>
#include <mutex>
#include <vector>
#include <deque>
#include <condition_variable>
#include <memory>
#include <atomic>
#include "../../protocol/MessageCodec/MessageCodec.h"
struct PendingFile{
    std::string sender;
    std::string filename;
    long long filesize=0;
    std::string targetType;
    std::string groupname;
    int fileid=-1;
};
class FileClient{
public:
    static FileClient& instance(){
        static FileClient client;
        return client;
    }
    void sendFile(int fd,int fileid,const std::string& filepath,const std::string& filename,const std::string& target,const std::string& targetType,const std::string& groupname,long long offset);
    void addPendingFile(const std::string& sender,const std::string& filename, long long size, const std::string& targetType,const std::string& groupname,int fileid);

    PendingFile getPendingFile(const std::string& sender,const std::string& filename);

    void setSendFilePath(const std::string& targetType,const std::string& target,const std::string& filename,const std::string& filepath);
    std::string getSendFilePath(const std::string& targetType,const std::string& target,const std::string& filename);

    void setUsername(const std::string& name);

    std::string getUsername();

    //接收文件
    void receiveFile(const FilePacket& packet,int fd);
    void connectionClosed(int fd);
private:

    struct ReceiveChunk{
        long long offset=0;
        std::string data;
    };

    struct ReceiveState{
        int fileid=-1;
        std::string filepath;
        long long filesize=0;
        std::atomic<int> fd{-1};
        std::deque<ReceiveChunk> chunks;
        size_t queuedBytes=0;
        bool workerStarted=false;
        std::mutex mutex;
        std::condition_variable condition;
    };

    FileClient(){}

private:

    std::string username_;

    std::map<std::string,PendingFile> pendingFiles_;
    std::map<std::string,std::string> sendFilePaths_;
    std::map<int,bool> sendingFiles_;
    std::map<std::string,std::shared_ptr<ReceiveState>> receiveStates_;


    std::mutex mutex_;

};
