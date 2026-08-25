#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <nlohmann/json.hpp>
using json=nlohmann::json;

class RedisManager{
  public:
    static RedisManager& instance();

    // 删除拷贝构造、赋值，禁止复制这个唯一对象
    RedisManager(const RedisManager&) = delete;
    RedisManager& operator=(const RedisManager&) = delete;

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

    //离线文件申请
    //保存离线文件的消息
    bool saveOfflineFileRequest(const std::string& username,const json& js);
    //获取离线文件
    std::vector<std::string>getOfflineFile(const std::string& username);
    //删除离线消息
    void clearOfflineFiles(const std::string& username);

    //验证码
    //保存验证码
    bool saveVerifyCode(const std::string& target,const std::string& code);
    //查询验证码
    std::string getVerifyCode(const std::string& target);
    //删除验证码
    bool deleteVerifyCode(const std::string& target);

    //离线群邀请
    bool saveOfflineGroupInvite(const std::string& username,const json& js);
    std::vector<std::string> getOfflineGroupInvite(const std::string& username);
    void clearOfflineGroupInvite(const std::string& username);

  private:
    // 私有构造、析构 → 外面不能 new RedisManager()
    RedisManager();
    ~RedisManager();

    void* redisContext_;
    std::mutex mutex_;
    //维持和 Redis 服务器之间通信通道的凭证
};
