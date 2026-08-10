#pragma once
#include <string>
#include <nlohmann/json.hpp>
using json = nlohmann::json;
class GroupRequestService{
    public:
      GroupRequestService();
      ~GroupRequestService();
      //查看群申请
      static json getRequestList(const json& js);
      //处理申请(同意/拒绝)
      static json handleGroupRequest(const json& js);
};