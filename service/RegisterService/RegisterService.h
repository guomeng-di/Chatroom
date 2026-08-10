#pragma once
#include <nlohmann/json.hpp>
using json=nlohmann::json;

class RegisterService{
    public:
      RegisterService();
      ~RegisterService();

      static json registerUser(const json& js);
};