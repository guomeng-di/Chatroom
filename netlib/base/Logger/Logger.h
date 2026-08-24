#ifndef LOGGER_H
#define LOGGER_H

#include <glog/logging.h>

// 统一日志宏
#define LOG_INFO LOG(INFO)
#define LOG_WARN LOG(WARNING)
#define LOG_ERROR LOG(ERROR)
#define LOG_FATAL LOG(FATAL)

// 初始化日志
void InitLogger(const char* program_name, const char* log_dir = "./log");

// 关闭日志
void CloseLogger();

#endif