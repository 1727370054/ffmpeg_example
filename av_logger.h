//
// Created by HWK15 on 2025/2/22.
//

#ifndef ALL_EXAMPLE_AV_LOGGER_H
#define ALL_EXAMPLE_AV_LOGGER_H

#include <sstream>
#include <string>

#define __STDC_CONSTANT_MACROS
#ifdef _WIN32
extern "C"
{
#include "libavutil/log.h"
};
#else
#ifdef ___cplusplus
extern "C"
{
#endif
#include "libavutil/log.h"
#ifdef ___cplusplus
};
#endif
#endif

//#define _DEBUG;

class AVLogger
{
public:
    enum LogLevel
    {
        Error = AV_LOG_ERROR,
        Warning = AV_LOG_WARNING,
        Info = AV_LOG_INFO,
        Debug = AV_LOG_DEBUG
    };

    AVLogger(LogLevel level) : level_(level)
    {
        av_log_set_callback(&AVLogger::custom_log_callback);
    }

    ~AVLogger()
    {
        stream_ << "\n";
        av_log(nullptr, level_, "%s", stream_.str().c_str());
        fflush(stdout);  // 立即刷新输出
    }

    template<typename T>
    AVLogger& operator<<(const T& value)
    {
        stream_ << value;
        return *this;
    }

    // 支持std::endl等操作符
    AVLogger& operator<<(std::ostream& (*manip)(std::ostream&))
    {
        stream_ << manip;
        return *this;
    }

private:
    static void custom_log_callback(void *ptr, int level, const char *fmt, va_list vargs);

    LogLevel level_;
    std::ostringstream stream_;
};

// 便捷宏定义
#ifdef _DEBUG
#define LOG_ERROR   AVLogger(AVLogger::Error)
    #define LOG_WARNING AVLogger(AVLogger::Warning)
    #define LOG_INFO    AVLogger(AVLogger::Info)
    #define LOG_DEBUG   AVLogger(AVLogger::Debug)
#else
// Release模式下可以禁用某些日志
#define LOG_ERROR   AVLogger(AVLogger::Error)
#define LOG_WARNING AVLogger(AVLogger::Warning)
#define LOG_INFO    AVLogger(AVLogger::Info)
#define LOG_DEBUG   if(0) AVLogger(AVLogger::Debug)
#endif

#endif //ALL_EXAMPLE_AV_LOGGER_H
