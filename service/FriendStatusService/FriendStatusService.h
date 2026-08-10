//专门负责:上下线时发送通知
#pragma once
#include <string>
#include <nlohmann/json.hpp>

using json=nlohmann::json;
class FriendStatusService{
    public:
      //通知好友 用户上线
      static void notifyOnline(const std::string& username);
      //通知好友 用户下线
      static void notifyOffline(const std::string& username);
};