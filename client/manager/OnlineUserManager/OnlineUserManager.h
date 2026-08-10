#pragma once

#include <unordered_map>
#include <string>
class TcpConnection;
class OnlineUserManager{
    public:
      OnlineUserManager();
      ~OnlineUserManager();

      void addUser(const std::string& username,TcpConnection* conn);//添加在线用户
      void removeUser(const std::string& username);//删除在线用户
      TcpConnection* getConnection(const std::string& username);//根据用户名找到连接
      bool isOnline(const std::string& username);//判断好友在线状态
    private:
      std::unordered_map<std::string,TcpConnection*> users_;
};
extern OnlineUserManager onlineUserManager;
 
