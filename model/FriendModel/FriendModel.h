//model负责：数据怎么保存和查询
#pragma once
#include <string>
#include <unordered_map>
#include <unordered_set>
class FriendModel{
    public:
      FriendModel();
      ~FriendModel();

      //添加好友
      bool addFriend(const std::string& user,const std::string& friendName);
      //判断好友关系
      bool isFriend(const std::string& user,const std::string& friendName);
      //获取好友列表
      std::unordered_set<std::string> getFriends(const std::string& user);
      //删除好友
      bool removeFriend(const std::string& user,const std::string& friendName);
      //删除所有好友(注销账号)
      bool removeAllFriends(const std::string& username);

    // private:
    //   static std::unordered_map<std::string,std::unordered_set<std::string>> friends_;
};
// 以前：

// FriendModel
//     |
//  unordered_map

// 现在：

// FriendModel
//     |
//  MySQL