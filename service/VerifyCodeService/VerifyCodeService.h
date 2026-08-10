#pragma once
#include <nlohmann/json.hpp>

class TcpConnection;
using json=nlohmann::json;
class VerifyCodeService{
    public:
      static json sendCode(const json& js);
};