//
// Created by HWK15 on 2025/2/25.
//

#include "demuxing_decoding.h"

MediaDecoder::MediaDecoder(const string &src_filename, const string &video_file, const string &audio_file)
        : _src_filename(src_filename)
{
    if (src_filename.empty() || video_file.empty() || audio_file.empty())
    {
        LOG_WARNING << "用法: 输入文件 视频输出文件 音频输出文件\n"
        "API示例程序，展示如何从输入文件读取帧。\n"
        "该程序从文件读取帧，解码它们，并将解码后的\n"
        "视频帧写入名为video_output_file的原始视频文件，将解码后的\n"
        "音频帧写入名为audio_output_file的原始音频文件。\n";
        return;
    }

    init();

    // 尝试初始化视频解码器
    try
    {
        videoDecoder = std::make_unique<VideoDecoder>(_format_context.get(), video_file);
    }
    catch (const std::exception& e)
    {
        std::cerr << "视频解码器初始化失败: " << e.what() << std::endl;
    }

    // 尝试初始化音频解码器
    try
    {
        audioDecoder = std::make_unique<AudioDecoder>(_format_context.get(), audio_file);
    }
    catch (const std::exception& e)
    {
        std::cerr << "音频解码器初始化失败: " << e.what() << std::endl;
    }

    // 检查是否至少有一个解码器成功初始化
    if (!videoDecoder && !audioDecoder)
    {
        throw std::runtime_error("无法初始化任何解码器");
    }
}

void MediaDecoder::init()
{
    int ret = 0;
    AVFormatContext* ctx = nullptr;
    ret = avformat_open_input(&ctx, _src_filename.c_str(), nullptr, nullptr);
    if (ret < 0)
    {
        LOG_ERROR << "无法打开媒体文件!" << Tools::avErrorToString(ret);
        return;
    }

    _format_context.reset(ctx);

    ret = avformat_find_stream_info(_format_context.get(), nullptr);
    if (ret < 0)
    {
        LOG_ERROR << "无法获取流信息!" << Tools::avErrorToString(ret);
        return;
    }

    // 分配帧对象
    _frame.reset(av_frame_alloc());
    if (!_frame)
    {
        LOG_ERROR << "无法分配帧结构!";
        return;
    }

    memset(_frame.get(), 0, sizeof(AVFrame));

    // 分配数据包
    _packet.reset(av_packet_alloc());
    if (!_packet)
    {
        LOG_ERROR << "无法分配包结构!";
        return;
    }

    memset(_packet.get(), 0, sizeof(AVPacket));

    av_dump_format(_format_context.get(), 0, _src_filename.c_str(), 0);
}

void MediaDecoder::decode()
{
    // 读取所有帧
    while (av_read_frame(_format_context.get(), _packet.get()) >= 0) {
        processPacket(_packet.get());
        av_packet_unref(_packet.get());
    }

    // 刷新解码器，获取缓冲的帧
    flushDecoders();

    std::cout << "解复用和解码成功完成" << std::endl;

    // 打印播放命令
    if (videoDecoder) {
        std::cout << "视频播放命令: " << videoDecoder->getPlayCommand() << std::endl;
    }

    if (audioDecoder) {
        std::cout << "音频播放命令: " << audioDecoder->getPlayCommand() << std::endl;
    }
}

void MediaDecoder::processPacket(const AVPacket *pkt)
{
    int ret = 0;

    if (pkt->stream_index == videoDecoder->getStreamIndex())
    {
        ret = videoDecoder->sendPacket(pkt);
        while (ret >= 0)
        {
            ret = videoDecoder->receiveFrame(_frame.get());
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            {
                break;
            }
            else if (ret < 0)
            {
                LOG_ERROR << "视频帧解码错误";
                break;
            }
        }
    }
    else if (pkt->stream_index == audioDecoder->getStreamIndex())
    {
        ret = audioDecoder->sendPacket(pkt);
        while (ret >= 0)
        {
            ret = audioDecoder->receiveFrame(_frame.get());
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            {
                break;
            }
            else if (ret < 0)
            {
                LOG_ERROR << "音频帧解码错误";
                break;
            }
        }
    }
}

void MediaDecoder::flushDecoders()
{
    // 刷新视频解码器
    if (videoDecoder)
    {
        int ret = videoDecoder->sendPacket(nullptr);
        while (ret >= 0) {
            ret = videoDecoder->receiveFrame(_frame.get());
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            }
        }
    }

    // 刷新音频解码器
    if (audioDecoder) {
        int ret = audioDecoder->sendPacket(nullptr);
        while (ret >= 0) {
            ret = audioDecoder->receiveFrame(_frame.get());
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            }
        }
    }
}


VideoDecoder::VideoDecoder(AVFormatContext* format_ctx, const string &dst_filename)
    : Decoder(format_ctx), _dst_filename(dst_filename)
{
    initCodecContext();

    _ofstream.open(dst_filename, ios::binary);
    if (!_ofstream.is_open())
    {
        LOG_ERROR << "打开文件失败 filename：" << dst_filename;
        return;
    }
}

void VideoDecoder::initCodecContext()
{
    int ret = av_find_best_stream(_format_ctx, AVMediaType::AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (ret < 0)
    {
        LOG_ERROR << "找不到视频流";
        return;
    }

    _stream_index = ret;
    _stream = _format_ctx->streams[_stream_index];

    // 查找解码器
    const AVCodec* decoder = avcodec_find_decoder(_stream->codecpar->codec_id);
    if (!decoder)
    {
        LOG_ERROR << "找不到合适的视频解码器";
        return;
    }

    // 分配解码器上下文
    _codec_context.reset(avcodec_alloc_context3(decoder));
    if (!_codec_context)
    {
        LOG_ERROR << "无法分配视频解码器上下文";
        return;
    }

    // 从流参数填充解码器上下文
    ret = avcodec_parameters_to_context(_codec_context.get(), _stream->codecpar);
    if (ret < 0)
    {
        LOG_ERROR << "无法将编解码器参数复制到上下文";
        return;
    }

    ret = avcodec_open2(_codec_context.get(), decoder, nullptr);
    if (ret < 0)
    {
        LOG_ERROR << "无法打开视频解码器";
        return;
    }

    // 获取视频属性
    _width = _codec_context->width;
    _height = _codec_context->height;
    _pix_fmt = _codec_context->pix_fmt;

    ret = av_image_alloc(_dest_data, _dest_line_size, _width, _height, _pix_fmt, 1);
    if (ret < 0)
    {
        LOG_ERROR << "无法分配图像缓冲区";
        return;
    }
    _dest_buf_size = ret;
}

void VideoDecoder::cleanup()
{
    if (_dest_data[0]) {
        av_freep(&_dest_data[0]);
    }
    if (_ofstream.is_open()) {
        _ofstream.close();
    }
}

VideoDecoder::~VideoDecoder()
{
    cleanup();
}

int VideoDecoder::sendPacket(const AVPacket *pkt)
{
    int ret = avcodec_send_packet(_codec_context.get(), pkt);
    if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
    {
        LOG_ERROR << "发送数据包到视频解码器时出错: "
                << Tools::avErrorToString(ret);
    }

    return ret;
}

int VideoDecoder::receiveFrame(AVFrame *frame)
{
    ///< EAGAIN(11): avcodec_send_packet() 还未提供足够的数据，解码器需要更多数据才能输出 AVFrame。
    ///< 在连续调用 avcodec_receive_frame() 时，可能会遇到 解码器内部缓存的帧已经取完 的情况，此时需要继续送入新的 AVPacket。

    ///< AVERROR_EOF: 表示解码器已经解码完所有数据，并且不会再有新的帧可供输出。
    ///< 解码器已经被 flush（即 avcodec_send_packet(codecCtx, nullptr) 之后）。所有缓冲的帧已经被取完，不会再有新的数据产生。

    int ret = avcodec_receive_frame(_codec_context.get(), frame);
    if (ret >= 0)
    {
        (void)processFrame(frame);
    }
    else if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
    {
    }
    else
    {
        LOG_ERROR << "读取数据包时出错: " << Tools::avErrorToString(ret);
    }

    return ret;
}

int VideoDecoder::processFrame(AVFrame *frame)
{
    // 检查帧尺寸和格式是否变化
    if (frame->width != _width || frame->height != _height ||
        frame->format != _pix_fmt) {
        LOG_ERROR << "错误: 视频帧参数变化，原始视频格式不支持";
        return -1;
    }

    if (!_dest_data[0] || _dest_buf_size <= 0 || !_ofstream.is_open()) {
        LOG_ERROR << "错误: 视频帧处理资源未正确初始化";
        return -1;
    }

    LOG_INFO << "视频帧 #" << _frame_count++
              << " coded_n:" << frame->coded_picture_number;

    av_image_copy(_dest_data, _dest_line_size, (const uint8_t**)(frame->data), frame->linesize, _pix_fmt, _width, _height);

    _ofstream.write(reinterpret_cast<char *>(_dest_data[0]), _dest_buf_size);

    return 0;
}

AudioDecoder::AudioDecoder(AVFormatContext *format_ctx, const string &dst_filename)
    : Decoder(format_ctx), _dst_filename(dst_filename)
{
    initCodecContext();

    _ofstream.open(dst_filename, ios::binary);
    if (!_ofstream.is_open())
    {
        LOG_ERROR << "打开文件失败 filename：" << dst_filename;
        return;
    }
}

void AudioDecoder::initCodecContext()
{
    int ret = av_find_best_stream(_format_ctx, AVMediaType::AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (ret < 0)
    {
        LOG_ERROR << "找不到音频流";
        return;
    }

    _stream_index = ret;
    _stream = _format_ctx->streams[_stream_index];

    // 查找解码器
    const AVCodec* decoder = avcodec_find_decoder(_stream->codecpar->codec_id);
    if (!decoder)
    {
        LOG_ERROR << "找不到合适的音频解码器";
        return;
    }

    // 分配解码器上下文
    _codec_context.reset(avcodec_alloc_context3(decoder));
    if (!_codec_context)
    {
        LOG_ERROR << "无法分配音频解码器上下文";
        return;
    }

    // 从流参数填充解码器上下文
    ret = avcodec_parameters_to_context(_codec_context.get(), _stream->codecpar);
    if (ret < 0)
    {
        LOG_ERROR << "无法将编解码器参数复制到上下文";
        return;
    }

    ret = avcodec_open2(_codec_context.get(), decoder, nullptr);
    if (ret < 0)
    {
        LOG_ERROR << "无法打开音频解码器";
        return;
    }
}

int AudioDecoder::sendPacket(const AVPacket *pkt)
{
    int ret = avcodec_send_packet(_codec_context.get(), pkt);
    if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
    {
        LOG_ERROR << "发送数据包到音频解码器时出错: "
                  << Tools::avErrorToString(ret);
    }

    return ret;
}

int AudioDecoder::receiveFrame(AVFrame *frame)
{
    int ret = avcodec_receive_frame(_codec_context.get(), frame);
    if (ret >= 0)
    {
        (void)processFrame(frame);
    }
    else if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
    {
    }
    else
    {
        LOG_ERROR << "读取数据包时出错: " << Tools::avErrorToString(ret);
    }

    return ret;
}

int AudioDecoder::processFrame(AVFrame *frame)
{
    ///< nb_samples: 单个声道（channel）中包含的采样点数量
    size_t unpaddedLinesize = frame->nb_samples * av_get_bytes_per_sample(static_cast<AVSampleFormat>(frame->format));

    LOG_INFO << "音频帧 #" << _frame_count++
              << " nb_samples:" << frame->nb_samples
              << " pts:" << frame->pts * av_q2d(_codec_context->time_base) << "s";

    _ofstream.write(reinterpret_cast<char*>(frame->extended_data[0]), unpaddedLinesize);

    return 0;
}

void AudioDecoder::cleanup()
{
    if (_ofstream.is_open()) {
        _ofstream.close();
    }
}

std::string AudioDecoder::getPlayCommand() const
{
    // 获取当前采样格式
    AVSampleFormat sampleFmt = _codec_context->sample_fmt;
    int channels = _codec_context->channels;
    std::string formatStr;

    // 检查是否是平面格式
    if (av_sample_fmt_is_planar(sampleFmt)) {
        std::cout << "警告: 解码器产生的采样格式是平面的 ("
                  << av_get_sample_fmt_name(sampleFmt)
                  << ")。此示例将只输出第一个声道。" << std::endl;
        sampleFmt = av_get_packed_sample_fmt(sampleFmt);
        channels = 1;
    }

    try {
        formatStr = getSampleFormatString(sampleFmt);
    } catch (const std::exception& e) {
        return "无法获取播放命令: " + std::string(e.what());
    }

    return "ffplay -f " + formatStr +
           " -ac " + std::to_string(channels) +
           " -ar " + std::to_string(_codec_context->sample_rate) +
           " " + _dst_filename;
}

std::string AudioDecoder::getSampleFormatString(enum AVSampleFormat sampleFmt) const
{
    struct
    {
        enum AVSampleFormat format;
        const char* formatName;
    }

    formatMap[] = {
            { AV_SAMPLE_FMT_U8,  "u8" },
            { AV_SAMPLE_FMT_S16, AV_NE("s16be", "s16le") },
            { AV_SAMPLE_FMT_S32, AV_NE("s32be", "s32le") },
            { AV_SAMPLE_FMT_FLT, AV_NE("f32be", "f32le") },
            { AV_SAMPLE_FMT_DBL, AV_NE("f64be", "f64le") }
    };

    for (const auto& entry : formatMap)
    {
        if (entry.format == sampleFmt)
        {
            return entry.formatName;
        }
    }

    throw std::runtime_error("不支持的采样格式: " +
                             std::string(av_get_sample_fmt_name(sampleFmt)));
}
