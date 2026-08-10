#pragma once
#include <nlohmann/json.hpp>
using json = nlohmann::json;
class HistoryService{
    public:
      HistoryService();
      ~HistoryService();
      // 私聊历史
      static json getPrivateHistory(const json& js);
      // 群聊历史
      static json getGroupHistory(const json& js);
};