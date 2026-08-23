#pragma once

#include <unordered_map>
#include <string>
class TcpConnection;
class OnlineUserManager{
    public:
      static OnlineUserManager& instance();

      void addUser(const std::string& username,TcpConnection* conn);//添加在线用户
      bool removeUser(const std::string& username, TcpConnection* conn = nullptr);//删除在线用户
      TcpConnection* getConnection(const std::string& username);//根据用户名找到连接
      bool isOnline(const std::string& username);//判断好友在线状态
    private:
      //构造私有化
      OnlineUserManager();
      //禁止复制
      OnlineUserManager(const OnlineUserManager&)=delete;
      OnlineUserManager& operator=(const OnlineUserManager&)=delete;
      std::unordered_map<std::string,TcpConnection*> users_;
};
extern OnlineUserManager onlineUserManager;
 
// Redis 存的只是【标记：这个人在线 / 离线】（一个布尔状态）；
// OnlineUserManager 存的是【这个人当前在这台服务器上的 TCP 连接对象 TcpConnection*】（能直接发消息、发包的通道）
// ✅ 发消息、转发文件通知，必须拿到 TcpConnection*，只能从 OnlineUserManager 拿，Redis 拿不到连接指针！
