#pragma once
#include <vector>

#include <string>
 class FriendBlockModel{
    public:
      //添加屏蔽
      bool addBlock(const std::string& username,const std::string& blockname);
      //取消屏蔽
      bool removeBlock(const std::string& username,const std::string& blockname);
      //判断是否屏蔽
      bool isBlocked(const std::string& username,const std::string& blockname);
      bool removeAllBlock(const std::string& user1,const std::string& user2);
      //获取用户屏蔽列表
    std::vector<std::string> getBlockList(
        const std::string& username
    );
 
    };