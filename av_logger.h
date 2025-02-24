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
namespace avlog
{

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
        if (!no_newline_)
        {
            stream_ << "\n";  // 默认换行
        }
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

    struct NoNewline {};  ///< 用于表示不换行

    // 重载 `<<` 让 nnl 起作用
    AVLogger& operator<<(NoNewline)
    {
        no_newline_ = true;  // 标记不换行
        return *this;
    }
private:
    static void custom_log_callback(void *ptr, int level, const char *fmt, va_list vargs);

    LogLevel level_;
    std::ostringstream stream_;
    bool no_newline_ = false;  ///< 是否换行
};

    extern AVLogger::NoNewline nnl;
};

// 便捷宏定义
#ifdef _DEBUG
#define LOG_ERROR   avlog::AVLogger(avlog::AVLogger::Error)
    #define LOG_WARNING avlog::AVLogger(avlog::AVLogger::Warning)
    #define LOG_INFO    avlog::AVLogger(avlog::AVLogger::Info)
    #define LOG_DEBUG   avlog::AVLogger(avlog::AVLogger::Debug)
#else
// Release模式下可以禁用某些日志
#define LOG_ERROR   avlog::AVLogger(avlog::AVLogger::Error)
#define LOG_WARNING avlog::AVLogger(avlog::AVLogger::Warning)
#define LOG_INFO    avlog::AVLogger(avlog::AVLogger::Info)
#define LOG_DEBUG   if(0) avlog::AVLogger(avlog::AVLogger::Debug)
#endif

#endif //ALL_EXAMPLE_AV_LOGGER_H
