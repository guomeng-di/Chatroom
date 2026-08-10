#pragma once
#include <string>
#include <vector>
struct GroupRequest{
    std::string groupname;
    std::string username;
    std::string time;
};

class GroupRequestModel{
    public:
      GroupRequestModel();
      ~GroupRequestModel();
      //添加申请
      bool addRequest(const std::string& groupname,const std::string& username);
      //查询某个群申请
      std::vector<GroupRequest> getRequests(const std::string& groupname);
      //1-处理完成后删除申请
      bool deleteRequest(const std::string& groupname,const std::string& username);
      //2-注销账号后删除申请记录
      bool removeGroupRequest(const std::string& username);
};
      