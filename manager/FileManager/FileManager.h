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
    long long receivedSize=0;//已经收到的字节
    int updateCount;//新增
};

class FileManager{
    public:
      static FileManager& instance(){
        static FileManager manager;
        return manager;
    }
      void startReceive(int fileid,const std::string& fromname,const std::string& filename,long long filesize,long long receivedSize);
      bool updateSize(int fileid,long long size);
      long long getReceivedSize(int fileid);
      bool exists(int fileid);
      bool checkFinish(int fileid);
    private:
      FileManager(){}
      std::map<int,ReceiveFileInfo> files_;
};