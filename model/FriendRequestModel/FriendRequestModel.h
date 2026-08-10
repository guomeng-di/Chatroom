//model负责：数据怎么保存和查询
#pragma once
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
struct FriendRequest{
    std::string from;
    std::string time;
};
class FriendRequestModel{
    public:
      FriendRequestModel();
      ~FriendRequestModel();

      //发送好友申请
      bool addRequest(const std::string& from,const std::string& to);
      //查询收到的好友申请
      std::vector<FriendRequest> getRequests(const std::string& username);
      //删除好友申请
      bool removeRequest(const std::string& from,const std::string& to);
      //删除所有好友申请(注销)
      bool removeAllRequests(const std::string& username);

//   private:
//       static std::unordered_map<std::string,std::unordered_set<std::string>> requests_;
  };