// Buffer的作用：
// TcpConnection
// recv()
//   |
//   v
// Buffer保存收到的数据
//   |
//   v
// 判断有没有完整消息
//   |
//   v
// 交给JSON解析
#pragma once
#include <cstdint>
#include <string>

class Buffer{
    public:
      Buffer();
      ~Buffer();

      //往buffer_里添加收到的数据
      void append(const char* data,size_t len);
      //取出全部数据
      //std::string retrieveAll();
      //当前缓冲区大小
      size_t size();
      //获取数据
      const char* peek();
      //删除前len长度数据
      void retrieve(size_t len);
      //判断是否有完整消息
      bool hasMessage();
      //取出一条完整消息返回，并在buffer删除
      std::string retrieveMessage();
      bool hasError();
    private:
    bool error_;
      std::string buffer_;//保存收到的数据
      static const uint32_t MAX_MESSAGE_SIZE = 10 * 1024 * 1024;
};
//peek查看Buffer当前有什么数据，但是不改变Buffer内容.主要用于：判断消息长度