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
};

#endif //ALL_EXAMPLE_GLOBAL_H
