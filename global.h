//
// Created by HWK15 on 2025/2/22.
//

#ifndef ALL_EXAMPLE_GLOBAL_H
#define ALL_EXAMPLE_GLOBAL_H

#include <string>
#include <iostream>
#include <memory>
#include <functional>
#include <fstream>

using namespace std;

#ifdef _WIN32
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavutil/avutil.h"
#include "libavutil/parseutils.h"
#include "libavutil/opt.h"
#include "libavutil/dict.h"
#include "libavformat/avformat.h"
#include "libavcodec/codec.h"
#include "libavformat/avio.h"
#include "libavutil/file.h"
#include "libavutil/samplefmt.h"
#include "libavutil/timestamp.h"
#include "libavutil/mathematics.h"
#include "libavutil/imgutils.h"
#include "libavfilter/buffersrc.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/avfilter.h"
#include "libavutil/time.h"
};
#else
#ifdef ___cplusplus
extern "C"
{
#endif
#include "libavcodec/avcodec.h"
#include "libavutil/avutil.h"
#include "libavutil/parseutils.h"
#include "libavutil/opt.h"
#include "libavutil/dict.h"
#include "libavformat/avformat.h"
#include "libavcodec/codec.h"
#include "libavformat/avio.h"
#include "libavutil/file.h"
#include "libavutil/samplefmt.h"
#include "libavutil/timestamp.h"
#ifdef ___cplusplus
};
#endif
#endif

#include "av_logger.h"

///< 用于RAII(资源获取即初始化)模式的智能指针自定义删除器
struct AVFormatContextDeleter
{
    void operator()(AVFormatContext* ctx)
    {
        if (ctx) avformat_close_input(&ctx);
    }
};

struct AVCodecContextDeleter
{
    void operator()(AVCodecContext* ctx)
    {
        if (ctx) avcodec_free_context(&ctx);
    }
};

struct AVPacketDeleter
{
    void operator()(AVPacket* packet)
    {
        if (packet) av_packet_free(&packet);
    }
};

struct AVFrameDeleter
{
    void operator()(AVFrame* frame)
    {
        if (frame) av_frame_free(&frame);
    }
};

struct AVFilterGraphDeleter
{
    void operator()(AVFilterGraph* graph)
    {
        if (graph) avfilter_graph_free(&graph);
    }
};

class Defer
{
public:
    using defer_func_t = std::function<void()>;

    Defer(defer_func_t defer_func) : _defer_func(defer_func) {}
    ~Defer()
    {
        if (_defer_func)
        {
            _defer_func();
        }
    }
private:
    defer_func_t _defer_func;
};

class Tools
{
public:
    static double r2d(AVRational r)
    {
        return r.den == 0 ? 0 : (double)r.num / (double)r.den;
    }

    static void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt, const char *tag)
    {
        AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;

        LOG_INFO << tag << ": " << "pts:" << av_ts2str(pkt->pts) << " pts_time:" << av_ts2timestr(pkt->pts, time_base)
        << " dts:" << av_ts2str(pkt->dts) <<" dts_time:" << av_ts2timestr(pkt->dts, time_base)
        << " duration:" <<  av_ts2str(pkt->duration) << " duration_time:" << av_ts2timestr(pkt->duration, time_base)
        << " stream_index:" << pkt->stream_index;
    }

     // 用于存储错误信息的辅助方法
     static std::string avErrorToString(int errnum)
     {
         char errbuf[AV_ERROR_MAX_STRING_SIZE];
         av_strerror(errnum, errbuf, AV_ERROR_MAX_STRING_SIZE);
         return std::string(errbuf);
     }
};

#endif //ALL_EXAMPLE_GLOBAL_H
