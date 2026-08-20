//管理正在接收的文件(单例函数)
#pragma once
#include <string>
#include <vector>
#include <map>
//acceptor保存正在接收的文件状态
struct ReceiveFileInfo{
    int fileid;
    std::string fromname;
    std::string filename;
    long long filesize=0;
    long long receivedSize=0;
    int lastBlock=-1;
    std::vector<int> receivedBlocks;
};

class FileManager{
    public:
      static FileManager& instance(){
        static FileManager manager;
        return manager;
    }
      void startReceive(int fileid,const std::string& fromname,const std::string& filename,long long filesize,bool resume=false);
      bool updateBlock(int fileid,int blockid,int size);
      bool checkFinish(int fileid);
      bool exists(int fileid);
      void resumeReceive(int fileid,const std::string& fromname,const std::string& filename,long long filesize,const std::vector<int>& receivedBlocks
);
bool hasBlock(int fileid,int blockid);
    private:
      FileManager(){}
      std::map<int,ReceiveFileInfo> files_;
};