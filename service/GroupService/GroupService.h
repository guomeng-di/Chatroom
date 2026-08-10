#pragma once
#include <string>
#include <nlohmann/json.hpp>
using json = nlohmann::json;
class GroupService{
    public:
      GroupService();
      ~GroupService();

      //创建群
      static json createGroup(const json& js);
      //加入群
      static json joinGroup(const json& js);
      //查看用户加入的群
      static json getGroupList(const json& js);
      //查看群成员
      static json getGroupMembers(const json& js);
      //退群
      static json leaveGroup(const json& js);
};
