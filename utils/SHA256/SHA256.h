#pragma once
#include <string>
class HashSHA256{
    public:
      static std::string encode(const std::string& str);
};
//接收原始字符串，返回 SHA256 十六进制哈希字符串