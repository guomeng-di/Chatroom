// 主要负责：

// 踢人
// 解散群
// 添加管理员
// 删除管理员
#pragma once

#include <string>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

class GroupManageService{
    public:
      GroupManageService();
      ~GroupManageService();
    //踢成员
    static json kickMember(const json& js);
    //解散群
    static json deleteGroup(const json& js);
    //添加管理员
    static json addAdmin(const json& js);
    //删除管理员
    static json removeAdmin(const json& js);
};
