#include "Logger.h"
#include <ctime>
#include <iomanip>
#include <sstream>
using namespace std;

Logger&Logger::instance(){
    static Logger logger;
    return logger;
}
//构造函数打开日志文件
Logger::Logger(){
    file_.open("server.log",ios::out);
    //file_.open("server.log",ios::app);//app->append追加模式
    if(!file_.is_open()) 
      cerr<<"open server.log failed"<<endl;
}
//析构
Logger::~Logger(){
    if(file_.is_open())
      file_.close();
}
//获取时间
string Logger::getTime(){
    time_t now=time(nullptr);//获取当前时间
    tm local;//struct tm存年月日分秒
    localtime_r(&now,&local);//时间存入local
    stringstream ss;
    ss<<1900+local.tm_year<<"-"<<
    setw(2)<<setfill('0')<<1+local.tm_mon<<"-"<<
    setw(2)<<setfill('0')<<local.tm_mday<<" "<<
    setw(2)<<setfill('0')<<local.tm_hour<<":"<<
    setw(2)<<setfill('0')<<1+local.tm_min<<":"<<
    setw(2)<<setfill('0')<<local.tm_sec;//2026-07-31 10:29:55
    return ss.str();
}
//INFO日志
void Logger::info(const string& msg){
    //加锁
    lock_guard<mutex> lock(mutex_);
    if(!file_.is_open()) return ;
    //file_往 server.log 文件写入内容
    file_<<"["<<getTime()<<"] "<<"[INFO] "<<msg<<endl;
    //[2026-07-31 22:10:35] [INFO] mysql connect success
    //立即刷新
    file_.flush();//flush() 强制把缓冲区内容立刻落地写到硬盘
}
//ERROR日志
void Logger::error(const string& msg){
    //加锁
    lock_guard<mutex> lock(mutex_);
    if(!file_.is_open()) return ;
    //file_往 server.log 文件写入内容
    file_<<"["<<getTime()<<"] "<<"[ERROR] "<<msg<<endl;
    //[2026-07-31 22:10:35] [INFO] mysql connect success
    //立即刷新
    file_.flush();
}