//
// Created by HWK15 on 2025/2/24.
//

#include "remuxer.h"

Remuxer::Remuxer()
{
    avformat_network_init();
    LOG_INFO << "avformat_network 初始化成功";
}

Remuxer::~Remuxer()
{
    if (_ifmt_ctx)
    {
        avformat_close_input(&_ifmt_ctx);
        avformat_free_context(_ifmt_ctx);
    }

    if (_ofmt_ctx && !(_ofmt_ctx->flags & AVFMT_NOFILE))
        avio_closep(&_ofmt_ctx->pb);

    if (_ofmt_ctx)
    {
        avformat_free_context(_ofmt_ctx);
    }

    av_freep(&_stream_mapping);

    avformat_network_deinit();
}

void Remuxer::start_remuxer(const string &in_filename, const string &out_filename)
{
    int ret, i;
    if ((ret = avformat_open_input(&_ifmt_ctx, in_filename.c_str(), nullptr, nullptr)) < -1)
    {
        LOG_ERROR << "avformat_open_input failed! error: " << av_strerror(ret, _err_buf, sizeof (_err_buf) - 1);
        return;
    }

    if ((ret = avformat_find_stream_info(_ifmt_ctx, nullptr)) < -1)
    {
        LOG_ERROR << "avformat_find_stream_info failed! error: " << av_strerror(ret, _err_buf, sizeof (_err_buf) - 1);
        return;
    }

    LOG_INFO << "==================================== av_dump_format:输入文件 =======================================";
    av_dump_format(_ifmt_ctx, 0, in_filename.c_str(), 0);
    LOG_INFO << "==================================== av_dump_format end =========================================";

    ret = avformat_alloc_output_context2(&_ofmt_ctx, nullptr, nullptr, out_filename.c_str());
    if (!_ofmt_ctx)
    {
        LOG_ERROR << "avformat_alloc_output_context2 failed! error: " << av_strerror(ret, _err_buf, sizeof (_err_buf) - 1);
        return;
    }

    _output_format = _ofmt_ctx->oformat;

    _stream_mapping_size = _ifmt_ctx->nb_streams;
    _stream_mapping = (int*)av_mallocz_array(_stream_mapping_size, sizeof (*_stream_mapping));
    if (!_stream_mapping)
    {
        LOG_ERROR << "_stream_mapping av_mallocz_array failed!";
        return;
    }

    // 开始拷贝codec参数和记录流索引
    for (int i = 0; i < _ifmt_ctx->nb_streams; ++i)
    {
        AVStream* in_stream = _ifmt_ctx->streams[i];
        AVCodecParameters* in_codecpar = in_stream->codecpar;

        // 过滤或者改变流索引
        // 这里过滤掉其他流
        if (in_codecpar->codec_type != AVMEDIA_TYPE_AUDIO &&
            in_codecpar->codec_type != AVMEDIA_TYPE_VIDEO &&
            in_codecpar->codec_type != AVMEDIA_TYPE_SUBTITLE)
        {
            _stream_mapping[i] = -1;
        }
        _stream_mapping[i] = _stream_index++;

        AVStream* out_stream = nullptr;
        out_stream = avformat_new_stream(_ofmt_ctx, nullptr);
        if (!out_stream)
        {
            LOG_ERROR << "Failed allocating output stream";
            ret = AVERROR_UNKNOWN;
            return;
        }

        ret = avcodec_parameters_copy(out_stream->codecpar, in_codecpar);
        if (ret < 0)
        {
            LOG_ERROR << "Failed to copy codec parameters";
            return;
        }

        out_stream->codecpar->codec_tag = 0;
    }

    LOG_INFO << "";

    LOG_INFO << "==================================== av_dump_format:输出文件 =======================================";
    av_dump_format(_ofmt_ctx, 0, out_filename.c_str(), 1);
    LOG_INFO << "==================================== av_dump_format end =========================================";
    LOG_INFO << "";

    if (!(_output_format->flags & AVFMT_NOFILE))
    {
        ret = avio_open(&_ofmt_ctx->pb, out_filename.c_str(), AVIO_FLAG_WRITE);
        if (ret < 0)
        {
            LOG_ERROR << "Could not open output file " << out_filename;
            return;
        }
    }

    ret = avformat_write_header(_ofmt_ctx, NULL);
    if (ret < 0)
    {
        LOG_ERROR << "avformat_write_header Error occurred when opening output file " << out_filename;
        return;
    }

    AVPacket pkt;
    while (true)
    {
        /// 读取音视频压缩包
        ret = av_read_frame(_ifmt_ctx, &pkt);
        if (ret < 0)
            break;

        /// 过滤包
        if (pkt.stream_index >= _stream_mapping_size ||
            _stream_mapping[pkt.stream_index] < 0)
        {
            av_packet_unref(&pkt);
            continue;
        }

        Tools::log_packet(_ifmt_ctx, &pkt, "in");

        AVStream *in_stream, *out_stream;
        in_stream  = _ifmt_ctx->streams[pkt.stream_index];

        pkt.stream_index = _stream_mapping[pkt.stream_index];
        out_stream = _ofmt_ctx->streams[pkt.stream_index];

        /* copy packet */
        pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
        pkt.pos = -1;
        Tools::log_packet(_ofmt_ctx, &pkt, "out");

        /// 交织写音视频包
        av_interleaved_write_frame(_ofmt_ctx, &pkt);

        av_packet_unref(&pkt);

        LOG_INFO << "";
    }

    av_write_trailer(_ofmt_ctx);
}
