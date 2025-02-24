//
// Created by HWK15 on 2025/2/22.
//

#ifndef ALL_EXAMPLE_AVIO_FILE_H
#define ALL_EXAMPLE_AVIO_FILE_H

#include "global.h"

/**
 * 自定义 AVIO
 */
class AVIOFile
{
public:
    AVIOFile();
    ~AVIOFile();

    bool open(const string& url);

    bool open_custom_avio(const string& url);
private:
    static int read_packet(void *opaque, uint8_t *buf, int buf_size);
    static int write_packet(void *opaque, uint8_t *buf, int buf_size);
    static int64_t seek_packet(void *opaque, int64_t offset, int whence);


    AVFormatContext* _av_format_context = nullptr;
    AVInputFormat* _input_format = nullptr;
    AVIOContext* _avio_context = nullptr;
    ifstream _ifstream;
    int64_t _total_bytes_read = 0;  // 使用 64 位整数以支持大文件
};


#endif //ALL_EXAMPLE_AVIO_FILE_H
