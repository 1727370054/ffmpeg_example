//
// Created by HWK15 on 2025/2/23.
//

#ifndef ALL_EXAMPLE_AVIO_READING_H
#define ALL_EXAMPLE_AVIO_READING_H

#include "global.h"

/**
 * 自定义数据来源, 内存映射
 */
class AVIOReading
{
public:
    bool open(const string& url);
private:
    static int read_packet(void *opaque, uint8_t *buf, int buf_size);
    static int write_packet(void *opaque, uint8_t *buf, int buf_size);
    static int64_t seek_packet(void *opaque, int64_t offset, int whence);


    AVFormatContext* _av_format_context = nullptr;
    AVInputFormat* _input_format = nullptr;
    AVIOContext* _avio_context = nullptr;

    struct buffer_data
    {
        uint8_t* ptr;              // 当前指针位置
        size_t size;              // 剩余大小
        uint8_t* original_ptr;    // 原始起始位置
        int64_t original_size;     // 原始总大小
    };

    buffer_data _buffer_data{nullptr, 0};
};


#endif //ALL_EXAMPLE_AVIO_READING_H
