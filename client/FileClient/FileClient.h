#pragma once
#include <string>
#include <map>
#include <mutex>
#include <vector>
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
private:

    FileClient(){}

    bool existBlock(int blockidconst, std::vector<int>& blocks);

private:

    std::string username_;

    std::map<std::string,PendingFile> pendingFiles_;
    std::map<std::string,std::string> sendFilePaths_;


    std::mutex mutex_;

};
