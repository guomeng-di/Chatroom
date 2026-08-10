#pragma once
#include <string>
#include <nlohmann/json.hpp>
using json=nlohmann::json;

class FriendBlockService{
    public:
      static json addBlock(const json& js);
      static json removeBlock(const json& js);
      static bool isBlocked(const std::string& username,const std::string& blockname);
};