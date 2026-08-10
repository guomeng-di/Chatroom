#pragma once
#include <string>
#include <nlohmann/json.hpp>
using json = nlohmann::json;
class FriendRequestService{
    public:
      FriendRequestService();
      ~FriendRequestService();
      //发送好友申请
      static json sendRequest(const json& js);
      //查询好友申请
      static json getRequestList(const json& js);
      //处理好友申请(同意/拒绝)
      static json handleRequest(const json& js);
};