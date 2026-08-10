#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <mutex>

class Logger{
    public:
      static Logger& instance();//全局唯一
      ~Logger();//防止再创建(全局唯一)
      void info(const std::string& msg);//普通消息
      void error(const std::string& msg);//错误消息

    private:
      Logger();
      

      Logger(const Logger&)=delete;//不得拷贝构造
      Logger& operator=(const Logger&)=delete;//禁用赋值运算符

    private:
      std::ofstream file_;//负责打开日志文件，把日志写入磁盘
      std::mutex mutex_;//互斥锁->每次写日志前上锁，写完解锁,保证多线程安全
      std::string getTime();//获取时间
};