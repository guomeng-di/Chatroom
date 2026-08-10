#pragma once
#include <nlohmann/json.hpp>
using json=nlohmann::json;
class ResetPasswordService{
    public:
      static json resetPassword(const json& js);
};