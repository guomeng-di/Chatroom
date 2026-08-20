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

      bool saveFileInfo(const std::string& from,const std::string& to,const std::string& groupname,const std::string& targetType,const std::string& filename,ll filesize);//保存文件请求
      //bool saveGroupFileInfo(const std::string& from,const std::string& groupname,const std::string& filename,ll filesize);
      bool updateFileStatus(const std::string& fromname,const std::string& toname,const std::string& filename,int status);
      std::string getFileName(const std::string& fromname,const std::string& toname);
      bool checkFileRequest(const std::string& fromname,const std::string& toname,const std::string& filename);
      bool checkGroupFileRequest(const std::string& fromname,const std::string& groupname,const std::string& filename);
      bool saveFileBlock(int fileid,const std::string& filename,const std::string& username,int blockid);
      std::vector<int> getReceivedBlocks(int fileid,const std::string& username);
      int getFileId(const std::string& fromname,const std::string& toname,const std::string& groupname,const std::string& targetType,const std::string& filename);
      //int getGroupFileId(const std::string& fromname,const std::string& groupname,const std::string& filename);
      bool blockExists(int fileid,const std::string& username,int blockid);
      std::vector<std::string> getUnfinishedFiles( const std::string& username);
      bool updateGroupFileStatus(const std::string& fromname,const std::string& groupname,const std::string& filename,int status);
      bool saveFileReceiver(int fileid,const std::string& receiver);
      bool updateFileReceiver(int fileid,const std::string& receiver,int status);
      std::vector<std::string> getFileReceivers(int fileid);
      bool checkAllReceiverFinish(int fileid);//修改file_info的status
      int getUnfinishedFileId(const std::string& fromname,const std::string& toname,const std::string& filename);
      long long getFileSize(int fileid);
};