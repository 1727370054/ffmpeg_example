//
// Created by HWK15 on 2025/2/24.
//

#ifndef ALL_EXAMPLE_REMUXER_H
#define ALL_EXAMPLE_REMUXER_H

#include "global.h"

class Remuxer
{
public:
    Remuxer();
    ~Remuxer();

    void start_remuxer(const string& in_filename, const string& out_filename);
private:
    AVFormatContext* _ifmt_ctx = nullptr;
    AVFormatContext* _ofmt_ctx = nullptr;
    AVOutputFormat* _output_format = nullptr;

    int *_stream_mapping = nullptr; // 记录输入和输出的流索引，可以手动过滤某一流, 或者改变流索引
    int _stream_index = 0;
    int _stream_mapping_size = 0;

    char _err_buf[1024] = { 0 };
};


#endif //ALL_EXAMPLE_REMUXER_H
