//
// Created by HWK15 on 2025/2/23.
//

#ifndef ALL_EXAMPLE_DEMUXTER_H
#define ALL_EXAMPLE_DEMUXTER_H

#include "global.h"

class Demuxer
{
public:
    Demuxer();
    ~Demuxer();

    void open_demuxer(const string& url);
    void open_demuxer_read_frame(const string& url);
private:
    AVFormatContext* _av_format_context = nullptr;
    AVInputFormat* _input_format = nullptr;
    AVIOContext* _avio_context = nullptr;
    char _err_buf[1024] = { 0 };
};


#endif //ALL_EXAMPLE_DEMUXTER_H
