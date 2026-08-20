#pragma once
#include <string>
class MD5{
    public:
      static std::string getFileMD5(const std::string& filepath);
};
//计算文件 MD5 哈希值