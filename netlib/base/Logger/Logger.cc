#include "Logger.h"

#include <sys/stat.h>

#include <sys/types.h>

#include <unistd.h>

#include <errno.h>

void InitLogger(const char* program_name,const char* log_dir){

    if(access(log_dir,F_OK) != 0){

        mkdir(log_dir,0755);

    }

    FLAGS_log_dir = log_dir;

    google::InitGoogleLogging(program_name);

    // 只记录WARNING、ERROR、FATAL
    FLAGS_minloglevel = 0;

    // 不输出到终端
    FLAGS_stderrthreshold = 4;

    // 日志文件最大大小(MB)
    FLAGS_max_log_size = 100;

    // 磁盘满停止写日志
    FLAGS_stop_logging_if_full_disk = true;

    // 输出时间
    FLAGS_timestamp_in_logfile_name = true;

    // 不显示终端颜色
    FLAGS_colorlogtostderr = false;

}


void CloseLogger(){

    google::ShutdownGoogleLogging();

}