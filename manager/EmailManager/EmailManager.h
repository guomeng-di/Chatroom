//EmailManager收到redis中的邮箱和验证码参数->发送给客户端
#pragma once
#include <string>
class EmailManager{
    public:
      static EmailManager& instance();
      static bool sendCode(const std::string& email,const std::string& code);
      private:
        // 私有化构造、拷贝，标准单例写法
    EmailManager() = default;
    EmailManager(const EmailManager&) = delete;
    EmailManager& operator=(const EmailManager&) = delete;
};
// EmailManager
//         |
//         |
//         ↓
//     SMTP服务器
//         |
//         |
//         ↓
//     收件人邮箱