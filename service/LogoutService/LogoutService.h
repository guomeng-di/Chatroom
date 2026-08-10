#pragma once
#include <nlohmann/json.hpp>
using json=nlohmann::json;
class LogoutService{
    public:
      static json logout(const json& js);
};