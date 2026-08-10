//定义消息格式,采用JSON
#pragma once
#include <nlohmann/json.hpp>

using json=nlohmann::json;
class JsonProtocol{
    public:
      //序列化
      static std::string encode(const json& js);
      //反序列化
      static json decode(const std::string& msg);
};