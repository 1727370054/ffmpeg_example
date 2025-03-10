/********************************************************************************
* @author: hwk
* @email: orionlink@163.com
* @date: 2025/3/1 18:08
* @version: 1.0
* @description: 
********************************************************************************/

#include "live_test.h"

LiveTest::LiveTest()
{
    avformat_network_init();
    LOG_INFO << "avformat_network 初始化成功";
}

LiveTest::~LiveTest()
{
    avformat_network_deinit();
}

void LiveTest::start_live(const string &in_filename, const string &out_url)
{
    int ret, i;
    int videoindex = -1;
    int frame_index = 0;

    _packet.reset(av_packet_alloc());
    if (!_packet)
    {
        LOG_ERROR << "av_packet_alloc failed!";
        return;
    }

    //输入（Input）
    AVFormatContext* ifmt_ctx = nullptr;
    if ((ret = avformat_open_input(&ifmt_ctx, in_filename.c_str(), 0, 0)) < 0)
    {
        LOG_ERROR << "Could not open input file";
        return;
    }

    _ifmt_ctx.reset(ifmt_ctx);

    if ((ret = avformat_find_stream_info(_ifmt_ctx.get(), 0)) < 0)
    {
        LOG_ERROR << "Failed to retrieve input stream information";
        return;
    }

    ret = av_find_best_stream(_ifmt_ctx.get(), AVMediaType::AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (ret < 0)
    {
        LOG_ERROR << "Failed to av_find_best_stream";
        return;
    }

    videoindex = ret;

    av_dump_format(_ifmt_ctx.get(), 0, in_filename.c_str(), 0);

    //输出（Output）
    AVFormatContext* ofmt_ctx = nullptr;
    ret = avformat_alloc_output_context2(&ofmt_ctx, nullptr, "flv", out_url.c_str());
    if (ret < 0 || !ofmt_ctx)
    {
        LOG_ERROR << "Failed to avformat_alloc_output_context2";
        return;
    }

    _ofmt_ctx.reset(ofmt_ctx);

    _out_fmt = _ofmt_ctx->oformat;

    ///<  根据输入流创建输出流（Create output AVStream according to input AVStream）
    for (int i = 0; i < _ifmt_ctx->nb_streams; ++i)
    {
        AVStream *in_stream = _ifmt_ctx->streams[i];
        AVCodecParameters* codec_par = in_stream->codecpar;

        AVStream *out_stream = nullptr;
        out_stream = avformat_new_stream(_ofmt_ctx.get(), nullptr);
        if (!out_stream)
        {
            LOG_ERROR << "Failed to avformat_new_stream";
            return;
        }

        ret = avcodec_parameters_copy(out_stream->codecpar, codec_par);
        if (ret < 0)
        {
            LOG_ERROR << "Failed to avcodec_parameters_copy";
            return;
        }

        out_stream->codecpar->codec_tag = 0;
    }

    LOG_INFO << "";
    av_dump_format(_ofmt_ctx.get(), 0, out_url.c_str(), 1);

    //打开输出URL（Open output URL）
    ///< 当输出到 RTMP 流时，FFmpeg 不需要创建本地文件，而是需要创建网络连接，因此 RTMP 复用器会设置 AVFMT_NOFILE 标志，并自行处理网络连接。
    if (!(_out_fmt->flags & AVFMT_NOFILE)) // 如果输出格式没有AVFMT_NOFILE标志，则需要程序显式打开输出文件
    {
        ret = avio_open(&ofmt_ctx->pb, out_url.c_str(), AVIO_FLAG_WRITE);
        if (ret < 0)
        {
            LOG_ERROR << "Could not open output URL " << out_url;
            return;
        }
    }

    ret = avformat_write_header(_ofmt_ctx.get(), nullptr);
    if (ret < 0)
    {
        LOG_ERROR << "avformat_write_header failed";
        return;
    }

    int64_t start_time = av_gettime();
    while (true)
    {
        ret = av_read_frame(_ifmt_ctx.get(), _packet.get());
        if (ret < 0)
        {
            LOG_ERROR << "av_read_frame failed";
            break;
        }

        ///< FIX：No PTS (Example: Raw H.264)
        ///< Simple Write PTS
        if (_packet->pts == AV_NOPTS_VALUE)
        {
            ///< time_base 单位是秒
            AVRational time_base = _ifmt_ctx->streams[videoindex]->time_base;

            // 获取每一帧持续的微秒数
            int64_t calc_duration = (double)AV_TIME_BASE / av_q2d( _ifmt_ctx->streams[videoindex]->r_frame_rate);

            ///< 流的时间基准单位 = 通过帧索引 × 帧持续时间计算当前帧的时间位置 / 时间基准转换为秒 × 内部时间基准单位
            _packet->pts = (double)(frame_index * calc_duration) / (double)(av_q2d(time_base) * AV_TIME_BASE);
            _packet->dts = _packet->pts;
            _packet->duration = (double)calc_duration/(double)(av_q2d(time_base) * AV_TIME_BASE);
        }

        ///< Important:Delay
        if(_packet->stream_index == videoindex)
        {
            AVRational time_base = ifmt_ctx->streams[videoindex]->time_base;
            AVRational time_base_q = {1,AV_TIME_BASE};

            int64_t pts_time = av_rescale_q(_packet->dts, time_base, time_base_q);
            int64_t now_time = av_gettime() - start_time;
            if (pts_time > now_time)
                av_usleep(pts_time - now_time);
        }

        AVStream *in_stream  = ifmt_ctx->streams[_packet->stream_index];
        AVStream *out_stream = ofmt_ctx->streams[_packet->stream_index];
        /* copy packet */
        //转换PTS/DTS（Convert PTS/DTS）

        /*
         * AV_ROUND_ZERO: 向零方向舍入（截断小数部分）。
         * AV_ROUND_INF: 向正无穷方向舍入（向上取整）。
         * AV_ROUND_DOWN: 向下取整（向负无穷方向舍入）。
         * AV_ROUND_UP: 向上取整（向正无穷方向舍入）。
         * AV_ROUND_NEAR_INF: 向最近的方向舍入（四舍五入）。
         * AV_ROUND_PASS_MINMAX: 确保结果在最小值和最大值之间，避免溢出。
         */
        _packet->pts = av_rescale_q_rnd(_packet->pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
        _packet->dts = av_rescale_q_rnd(_packet->dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
        _packet->duration = av_rescale_q(_packet->duration, in_stream->time_base, out_stream->time_base);
        _packet->pos = -1;

        //Print to Screen
        if(_packet->stream_index == videoindex)
        {
            LOG_INFO << "Send " << frame_index << " video frames to output URL";
            frame_index++;
        }

        ret = av_interleaved_write_frame(ofmt_ctx, _packet.get());

        if (ret < 0)
        {
            LOG_ERROR << "Error muxing packet";
            break;
        }

        av_packet_unref(_packet.get());
    }

    av_write_trailer(_ofmt_ctx.get());
}
