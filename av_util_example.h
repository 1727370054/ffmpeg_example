//
// Created by HWK15 on 2025/2/22.
//

#ifndef ALL_EXAMPLE_AV_UTIL_EXAMPLE_H
#define ALL_EXAMPLE_AV_UTIL_EXAMPLE_H

#include "global.h"

class AVUtilExample
{
public:
    AVUtilExample();
    void test_log();
    void test_dictionary();
    void test_parse_util();
private:
    static void custom_log_callback(void *ptr, int level, const char *fmt, va_list vargs);
};


#endif //ALL_EXAMPLE_AV_UTIL_EXAMPLE_H
