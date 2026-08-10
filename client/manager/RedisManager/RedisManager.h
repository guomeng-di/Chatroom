#pragma once
#include <string>
#include <vector>
class RedisManager{
    public:
      RedisManager();
      ~RedisManager();
      //连接redis
      bool connect();
     //私聊
      //保存离线消息
      bool saveOfflineMessage(const std::string& username,const std::string& message);
      //获取离线消息
      std::vector<std::string>getOfflineMessage(const std::string& username);
      //删除离线消息
      void clearOfflineMessage(const std::string& username);

     //群聊
      bool saveGroupOfflineMessage(const std::string& username,const std::string& message);
      std::vector<std::string> getGroupOfflineMessage(const std::string& username);
      void clearGroupOfflineMessage(const std::string& username);

      //设置在线
      bool setOnline(const std::string& username);
      //设置离线
      bool setOffline(const std::string& username);
      //查询在线
      bool isOnline(const std::string& username);

    //验证码
      //保存验证码
      bool saveVerifyCode(const std::string& target,const std::string& code);
      //查询验证码
      std::string getVerifyCode(const std::string& target);
      //删除验证码
      bool deleteVerifyCode(const std::string& target);
      

    private:
      void* redisContext_;
      //维持和 Redis 服务器之间通信通道的凭证
};