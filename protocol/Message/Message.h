// 消息头
// 消息长度
// 消息封装
// +----------------+-----------------------+
// | message length |      json数据          |
// |    4字节       |      N字节             |
// +----------------+-----------------------+
// [00000030][{"msgid":1,"name":"jack"}]
#pragma once

#include <string>

class Message{
    public:
      Message();
      ~Message();

      //设置消息内容
      void setBody(const std::string& body);
      //获取消息长度
      int length();
      //获取消息内容
      std::string body();

    private:
      int length_;
      std::string body_;
};