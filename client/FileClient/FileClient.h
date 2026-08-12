#pragma once
#include <string>
class FileClient{
    public:
      static FileClient& instance(){
        static FileClient client;
        return client;
      }
      void sendFile(int fd,const std::string& filename,const std::string& toname);
      void setPendingFile(const std::string& sender,const std::string& filename,long long size);
      std::string getPendingFileSender();
      std::string getPendingFilename();
      long long getPendingFileSize();
      void setUsername(const std::string& name);
      private:
      std::string username_;
      std::string pendingFileSender_;
      std::string pendingFilename_;
      long long pendingFileSize_;
};