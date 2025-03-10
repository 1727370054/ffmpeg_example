//
// Created by HWK15 on 2025/3/1.
//

#ifndef ALL_EXAMPLE_FILTERING_VIDEO_H
#define ALL_EXAMPLE_FILTERING_VIDEO_H

#include "global.h"

class FilteringVideo
{
public:
    FilteringVideo();
    ~FilteringVideo();

    /**
     * @brief 开始过滤输入文件的视频流，过滤完成之后输出到输出文件中
     */
    void start_filtering(const string& in_filename, const string& out_filename, const string& filter_descr);
private:
    /**
     * @brief 开始打开输入文件，并解封装解码
     */
    int open_input_file();

    int init_filters();

    void write_frame();
private:
    string _in_filename;
    string _out_filename;
    string _filter_descr;

    ofstream _ofstream;

    int _video_index;
    unique_ptr<AVCodecContext, AVCodecContextDeleter> _codec_context;
    unique_ptr<AVFormatContext, AVFormatContextDeleter> _format_context;
    unique_ptr<AVFrame, AVFrameDeleter> _frame;
    unique_ptr<AVFrame, AVFrameDeleter> _filt_frame;
    unique_ptr<AVPacket, AVPacketDeleter> _packet;
    unique_ptr<AVFilterGraph, AVFilterGraphDeleter> _filter_graph;

    AVFilterContext *_buffersink_ctx = nullptr;
    AVFilterContext *_buffersrc_ctx = nullptr;
};


#endif //ALL_EXAMPLE_FILTERING_VIDEO_H
