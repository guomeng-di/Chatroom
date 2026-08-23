#pragma once
#include <string>
#include <vector>
typedef long long ll;
#include <nlohmann/json.hpp>
using json=nlohmann::json;
class FileModel{
    public:
      FileModel();
      ~FileModel();

      bool saveFileInfo(const std::string& from,const std::string& to,const std::string& groupname,const std::string& targetType,const std::string& filename,ll filesize,const std::string& filepath);//保存文件请求
      bool updateFileStatus(const std::string& fromname,const std::string& toname,const std::string& filename,int status);
      std::string getFileName(const std::string& fromname,const std::string& toname);
      std::string getFilePath(int fileid);
      bool checkFileRequest(const std::string& fromname,const std::string& toname,const std::string& filename);
      bool checkGroupFileRequest(const std::string& fromname,const std::string& groupname,const std::string& filename);
      int getFileId(const std::string& fromname,const std::string& toname,const std::string& groupname,const std::string& targetType,const std::string& filename);
      long long getFileSize(int fileid);
      bool saveFileReceiver(int fileid,const std::string& receiver);//初始化接收记录
      bool updateFileReceiver(int fileid,const std::string& receiver,int status);
      bool updateFilePath(int fileid,const std::string& filepath);
    //更新某个接收者已经收到大小
    bool updateReceivedSize(int fileid,const std::string& receiver,long long size);
    //查询某个接收者已经收到大小
    long long getReceivedSize(int fileid,const std::string& receiver);
    std::vector<std::string> getFileReceivers(int fileid);
    bool checkAllReceiverFinish(int fileid);
    std::vector<std::string> getUnfinishedFiles(const std::string& username);
    int getUnfinishedFileId(const std::string& fromname,const std::string& toname,const std::string& filename);
    bool getUnfinishedFileInfo(const std::string& receiver,const std::string& filename,std::string& fromname,int& fileid,long long& filesize);
};
