//
// Created by HWK15 on 2025/3/1.
//

#include "filtering_video.h"

FilteringVideo::FilteringVideo()
{
    avformat_network_init();
    LOG_INFO << "avformat_network 初始化成功";
}

FilteringVideo::~FilteringVideo()
{
    avformat_network_deinit();
}

void FilteringVideo::start_filtering(const string &in_filename, const string &out_filename, const string &filter_descr)
{
    _in_filename = in_filename;
    _out_filename = out_filename;
    _filter_descr = filter_descr;

    _ofstream.open(out_filename, ios::binary);
    if (!_ofstream.is_open())
    {
        LOG_ERROR << "文件打开失败 filename: " << out_filename;
        return;
    }

    _packet.reset(av_packet_alloc());
    if (!_packet)
    {
        LOG_ERROR << "packet 分配失败";
        return;
    }

    _frame.reset(av_frame_alloc());
    _filt_frame.reset(av_frame_alloc());
    if (!_frame || !_filt_frame)
    {
        LOG_ERROR << "frame 分配失败";
        return;
    }

    int ret = 0;
    if ((ret = open_input_file()) < 0)
    {
        LOG_ERROR << "open_input_file 失败 error: " << av_err2str(ret);
        return;
    }
    if ((ret = init_filters()) < 0)
    {
        LOG_ERROR << "init_filters 失败 error: " << av_err2str(ret);
        return;
    }

    /* read all packets */
    while (true)
    {
        ret = av_read_frame(_format_context.get(), _packet.get());
        if (ret < 0)
            break;

        if (_packet->stream_index == _video_index)
        {
            ret = avcodec_send_packet(_codec_context.get(), _packet.get());
            if (ret < 0)
                break;

            while (ret >= 0)
            {
                ret = avcodec_receive_frame(_codec_context.get(), _frame.get());
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                    break;

                else if (ret < 0) {
                    LOG_ERROR << "Error while receiving a frame from the decoder";
                    return;
                }

                _frame->pts = _frame->best_effort_timestamp;

                if (av_buffersrc_add_frame_flags(_buffersrc_ctx, _frame.get(), AV_BUFFERSRC_FLAG_KEEP_REF))
                {
                    LOG_ERROR << "Error while feeding the filtergraph";
                    break;
                }

                while (true)
                {
                    ret = av_buffersink_get_frame(_buffersink_ctx, _filt_frame.get());
                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                    {
                        break;
                    }
                    else if (ret < 0)
                        return;

                    static int count = 0;
                    LOG_INFO << "Filtered frame #" << ++count
                             << " PTS=" << _filt_frame->pts
                             << " Resolution=" << _filt_frame->width << "x" << _filt_frame->height;

                    if (count % 50 == 0) {
                        LOG_INFO << "Processed " << count << " frames so far";
                    }

                    write_frame();
                    av_frame_unref(_filt_frame.get());
                }
                av_frame_unref(_frame.get());
            }
        }
        av_packet_unref(_packet.get());
    }
}

int FilteringVideo::open_input_file()
{
    int ret = 0;
    AVFormatContext* ctx = nullptr;
    ret = avformat_open_input(&ctx, _in_filename.c_str(), nullptr, nullptr);
    if (ret < 0)
    {
        return ret;
    }
    _format_context.reset(ctx);

    ret = avformat_find_stream_info(_format_context.get(), nullptr);
    if (ret < 0)
    {
        return ret;
    }

    AVCodec * decoder = nullptr;

    ///< 这个decoder可以在这里直接传递进去
    ret = av_find_best_stream(_format_context.get(), AVMediaType::AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (ret < 0)
    {
        return ret;
    }

    _video_index = ret;

    decoder = avcodec_find_decoder(_format_context->streams[_video_index]->codecpar->codec_id);
    if (!decoder)
    {
        LOG_ERROR << "找不到合适的视频解码器";
        return ret;
    }

    _codec_context.reset(avcodec_alloc_context3(decoder));
    if (!_codec_context)
    {
        LOG_ERROR << "视频解码上下文分配失败";
        return ret;
    }

    ret = avcodec_parameters_to_context(_codec_context.get(), _format_context->streams[_video_index]->codecpar);
    if (ret < 0)
    {
        LOG_ERROR << "avcodec_parameters_to_context 失败";
        return ret;
    }

    ret = avcodec_open2(_codec_context.get(), decoder, nullptr);
    if (ret < 0)
    {
        LOG_ERROR << "avcodec_open2 失败";
        return ret;
    }

    return ret;
}

int FilteringVideo::init_filters()
{
    int ret = 0;
    const AVFilter* buffersrc = avfilter_get_by_name("buffer");
    const AVFilter* buffersink = avfilter_get_by_name("buffersink");
    AVFilterInOut *inputs = avfilter_inout_alloc();
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVRational time_base = _format_context->streams[_video_index]->time_base;
    enum AVPixelFormat pix_fmts[] = { AV_PIX_FMT_GRAY8, AV_PIX_FMT_NONE };

    bool is_free = false;
    Defer defer([this, &is_free, &inputs, &outputs]()
    {
        if (is_free && inputs && outputs)
        {
            avfilter_inout_free(&inputs);
            avfilter_inout_free(&outputs);
        }
    });

    _filter_graph.reset(avfilter_graph_alloc());
    if (!outputs || !inputs || !_filter_graph)
    {
        ret = AVERROR(ENOMEM);
        is_free = true;
        return ret;
    }

    string args = "";
    stringstream ss;
    ss << "video_size=" << _codec_context->width << "x" << _codec_context->height
    << ":pix_fmt=" << _codec_context->pix_fmt << ":time_base=" << time_base.num << "/" << time_base.den
    << ":pixel_aspect=" << _codec_context->sample_aspect_ratio.num << "/" << _codec_context->sample_aspect_ratio.den;

    args = ss.str();

    LOG_INFO << "args: " << args;

    ret = avfilter_graph_create_filter(&_buffersrc_ctx, buffersrc, "in", args.c_str(), nullptr, _filter_graph.get());
    if (ret < 0) {
        LOG_ERROR << "Cannot create buffer source";
        is_free = true;
        return ret;
    }

    ret = avfilter_graph_create_filter(&_buffersink_ctx, buffersink, "out", "", nullptr, _filter_graph.get());
    if (ret < 0) {
        LOG_ERROR << "Cannot create buffer sink";
        is_free = true;
        return ret;
    }

    outputs->name = av_strdup("in");
    outputs->filter_ctx = _buffersrc_ctx;
    outputs->pad_idx = 0;
    outputs->next = nullptr;

    inputs->name = av_strdup("out");
    inputs->filter_ctx = _buffersink_ctx;
    inputs->pad_idx = 0;
    inputs->next = nullptr;

    ret = avfilter_graph_parse_ptr(_filter_graph.get(), _filter_descr.c_str(), &inputs, &outputs, nullptr);
    if (ret < 0)
    {
        LOG_ERROR << "avfilter_graph_parse_ptr failed";
        is_free = true;
        return ret;
    }

    ret = avfilter_graph_config(_filter_graph.get(),nullptr);
    if (ret < 0)
    {
        LOG_ERROR << "avfilter_graph_config failed";
        is_free = true;
        return ret;
    }

    return ret;
}

void FilteringVideo::write_frame()
{
    if (!_ofstream.is_open())
    {
        LOG_ERROR << "文件未打开";
        return;
    }

    static int printf_flag = 0;
    if(!printf_flag)
    {
        printf_flag = 1;
        LOG_INFO << "filter_frame: widht=" <<_filt_frame->width << " height=" << _filt_frame->height;

        if(_filt_frame->format==AV_PIX_FMT_YUV420P)
        {
            LOG_INFO << "format is yuv420p";
        }
        else
        {
            LOG_INFO << "format is = " << _filt_frame->format;
        }

    }

    _ofstream.write((char*)_filt_frame->data[0], _filt_frame->width * _filt_frame->height * 1);
    _ofstream.write((char*)_filt_frame->data[1], (_filt_frame->width / 2) * (_filt_frame->height / 2) * 1);
    _ofstream.write((char*)_filt_frame->data[2], (_filt_frame->width / 2) * (_filt_frame->height / 2) * 1);
}
