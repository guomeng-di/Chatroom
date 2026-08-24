#include "Logger.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

void InitLogger(const char* program_name, const char* log_dir){
    // 创建日志目录
    if(access(log_dir, F_OK) != 0){
        mkdir(log_dir,0755);
    }
    // 设置日志目录
    FLAGS_log_dir = log_dir;
    // 初始化glog
    google::InitGoogleLogging(program_name);
    // INFO及以上全部输出
    FLAGS_minloglevel = 0;
    // 输出到终端
    FLAGS_stderrthreshold = 0;
    // 日志文件最大大小(MB)
    FLAGS_max_log_size = 100;
    // 磁盘满停止写日志
    FLAGS_stop_logging_if_full_disk = true;
    // 输出时间
    FLAGS_timestamp_in_logfile_name = true;
    // 日志格式
    FLAGS_colorlogtostderr = true;
    LOG_INFO << "Logger initialized";
}


void CloseLogger(){
    LOG_INFO << "Logger shutdown";
    google::ShutdownGoogleLogging();
}