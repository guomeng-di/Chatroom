#pragma once
#include <string>
#include <map>
#include <vector>
#include "../../protocol/MessageCodec/MessageCodec.h"
#include "../../manager/FileManager/FileManager.h"
//acceptor保存别人发来的文件请求
struct PendingFile{
  std::string sender;
  std::string filename;
  long long filesize;
  std::string targetType;
  std::string groupname;
};

class FileClient{
    public:
      static FileClient& instance(){
        static FileClient client;
        return client;
      }
      void sendFile(int fd,int fileid,const std::string& filename,const std::string& target,const std::string& targetType,const std::string& groupname,std::vector<int> blocks);
      //待接收请求+1
      void addPendingFile(const std::string& sender,const std::string& filename,long long size,const std::string& targetType,const std::string& groupname);
      PendingFile getPendingFile(const std::string& sender,const std::string& filename);
      void setUsername(const std::string& name);
      std::string getUsername();
      void receiveFile(const FilePacket& packet,int fd);
      private:
      std::string username_;
      std::map<std::string,PendingFile> pendingFiles_;
      private:
      bool existBlock(int blockid,const std::vector<int>& blocks);
};