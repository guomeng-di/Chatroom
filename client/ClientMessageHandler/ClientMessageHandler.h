//收到服务器消息以后，根据msgid处理
#pragma once
#include <string>
#include <nlohmann/json.hpp>

using json=nlohmann::json;
class ClientMessageHandler{
    public:
      static void handle(const json& js,int fd);
};