#pragma once
#include <string>
 class FriendBlockModel{
    public:
      //添加屏蔽
      bool addBlock(const std::string& username,const std::string& blockname);
      //取消屏蔽
      bool removeBlock(const std::string& username,const std::string& blockname);
      //判断是否屏蔽
      bool isBlocked(const std::string& username,const std::string& blockname);
 };