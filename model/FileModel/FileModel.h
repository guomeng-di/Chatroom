#pragma once
#include <string>
typedef long long ll;

class FileModel{
    public:
      FileModel();
      ~FileModel();

      bool saveFileInfo(const std::string& from,const std::string& to,const std::string& filename,ll filesize);//保存文件请求
      bool updateFileStatus(const std::string& fromname,const std::string& toname,int status);
      std::string getFileName(const std::string& fromname,const std::string& toname);
};