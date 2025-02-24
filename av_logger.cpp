//
// Created by HWK15 on 2025/2/22.
//

#include "av_logger.h"

void AVLogger::custom_log_callback(void *ptr, int level, const char *fmt, va_list vargs)
{
    // 根据日志级别设置颜色
    const char *color = "";
    switch (level)
    {
        case AV_LOG_PANIC:
        case AV_LOG_FATAL:
        case AV_LOG_ERROR:
            color = "\033[31m";    // 红色
            break;
        case AV_LOG_WARNING:
            color = "\033[33m";    // 黄色
            break;
        case AV_LOG_INFO:
            color = "\033[32m";    // 绿色
            break;
        case AV_LOG_DEBUG:
            color = "\033[36m";     // 青色
#ifdef _DEBUG
            break;
#else
            return;
#endif
        default:
            color = "\033[0m";     // 直接不打印其他级别的
            return;
    }


    printf("%s", color);                     // 设置颜色
    vprintf(fmt, vargs);            // 打印日志
    printf("\033[0m");                       // 恢复默认颜色
    fflush(stdout);                            // 立即刷新输出
}
