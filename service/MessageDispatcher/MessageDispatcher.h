//根据msgid，把消息交给对应Service
#pragma once
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <string>
class TcpConnection;

using json=nlohmann::json;

class MessageDispatcher{
    public:
      MessageDispatcher();
      ~MessageDispatcher();
      static void dispatch(const json& js,TcpConnection* conn);
};