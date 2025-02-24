//
// Created by HWK15 on 2025/2/22.
//

#include "av_util_example.h"

AVUtilExample::AVUtilExample()
{
    av_log_set_callback(&AVUtilExample::custom_log_callback);
}

void AVUtilExample::test_log()
{
    AVFormatContext *formatContext = nullptr;
    formatContext = avformat_alloc_context();
    if (formatContext == nullptr) return;

    printf("=============================== avlog =================================\n");
    av_log(formatContext, AV_LOG_PANIC, "Panic: Something went really wrong and we will crash now.\n");
    av_log(formatContext, AV_LOG_FATAL, "Fatal: Something went wrong and recovery is not possible.\n");
    av_log(formatContext, AV_LOG_ERROR, "Error: Something went wrong and cannot losslessly be recovered.\n");
    av_log(formatContext,AV_LOG_WARNING, "Warning: This may or may not lead to problems.\n");
    av_log(formatContext,AV_LOG_INFO, "Info: Standard information.\n");
    av_log(formatContext,AV_LOG_VERBOSE, "Verbose: Detailed information.\n");
    av_log(formatContext,AV_LOG_DEBUG, "Debug: Stuff which is only useful for libav* developers.\n");
    printf("=======================================================================\n");

    avformat_free_context(formatContext);
}

void AVUtilExample::test_dictionary()
{
    AVDictionary* dictionary = nullptr;
    AVDictionaryEntry *entry = nullptr;

    av_dict_set(&dictionary, "name", "zhangsan", 0);
    av_dict_set(&dictionary, "age", "22", 0);
    av_dict_set(&dictionary, "gender", "man", 0);
    char *key = av_strdup("email"); // 重新分配内存，char *key = "email"; 是只读的 av_dict_set 会管理传入的内存
    char *value = av_strdup("123456@126.com");
    av_dict_set(&dictionary, key, value, AV_DICT_DONT_STRDUP_KEY | AV_DICT_DONT_STRDUP_VAL);

    printf("=============================== dictionary =================================\n");

    cout << "dictionary count: " << av_dict_count(dictionary) << endl;

    while ((entry = av_dict_get(dictionary, "", entry, AV_DICT_IGNORE_SUFFIX)) != nullptr)
    {
        cout << "key: " << entry->key << " | " << entry->value << endl;
    }

    entry = av_dict_get(dictionary, "email", entry, AV_DICT_IGNORE_SUFFIX);
    cout << "email is: " << entry->value << endl;

    printf("=======================================================================\n");

    av_dict_free(&dictionary);
}

void AVUtilExample::test_parse_util()
{
    printf("=============================== Parse Video Size =================================\n");
    int width_out = 0, height_out = 0;

    char * input_str = strdup("1920x1080");
    av_parse_video_size(&width_out, &height_out, input_str);
    printf("str: 1920x1080 - w:%4d | h:%4d\n",width_out,height_out);

    input_str = strdup("vga");//640x480(4:3)
    av_parse_video_size(&width_out, &height_out, input_str);
    printf("str: vga - w:%4d | h:%4d\n",width_out,height_out);

    strcpy(input_str,"hd1080");//high definition
    av_parse_video_size(&width_out, &height_out, input_str);
    printf("str: hd1080 - w:%4d | h:%4d\n",width_out,height_out);

    strcpy(input_str,"ntsc");//ntsc(N制720x480）
    av_parse_video_size(&width_out, &height_out, input_str);
    printf("str: ntsc - w:%4d | h:%4d\n",width_out,height_out);

    strcpy(input_str,"pal");// pal（P制720x576）
    av_parse_video_size(&width_out, &height_out, input_str);
    printf("str: pal - w:%4d | h:%4d\n",width_out,height_out);

    printf("=============================== Parse Frame Rate =================================\n");
    AVRational rational = {0, 0};
    strcpy(input_str,"15/1");
    av_parse_video_rate(&rational,input_str);
    printf("framerate:%d/%d\n",rational.num,rational.den);

    strcpy(input_str,"pal");//fps:25/1
    av_parse_video_rate(&rational,input_str);
    printf("pal - framerate:%d/%d\n",rational.num,rational.den);

    printf("=============================== Parse Time =================================\n");
    int64_t output_timeval;//单位：微妙， 1S=1000MilliSeconds, 1MilliS=1000MacroSeconds
    strcpy(input_str,"00:01:01");
    av_parse_time(&output_timeval,input_str,1);
    printf("microseconds:%lld\n",output_timeval);
    printf("====================================\n");
}

void AVUtilExample::custom_log_callback(void *ptr, int level, const char *fmt, va_list vargs)
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
        default:
            color = "\033[0m";     // 默认色
    }

    printf("%s", color);           // 设置颜色
    vprintf(fmt, vargs);           // 打印日志
    printf("\033[0m");            // 恢复默认颜色
    fflush(stdout);               // 立即刷新输出
}
