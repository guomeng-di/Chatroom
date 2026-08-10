#pragma once
#include <nlohmann/json.hpp>
using json=nlohmann::json;
class FriendService{
    public:
      FriendService();
      ~FriendService();

      //添加好友
      static json addFriend(const json& js);
      //查询好友列表
      static json getFriendList(const json& js);
      //删除好友
      static json deleteFriend(const json& js);
};