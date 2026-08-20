#pragma once
#include <string>
class SocketUtil{
    public:
      static bool sendAll(int fd,const std::string& data);
};