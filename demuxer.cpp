//
// Created by HWK15 on 2025/2/23.
//

#include "demuxer.h"

Demuxer::Demuxer()
{
    avformat_network_init();
    LOG_INFO << "avformat_network 初始化成功";
}

Demuxer::~Demuxer()
{
    if (_av_format_context)
    {
        avformat_close_input(&_av_format_context);
        avformat_free_context(_av_format_context);
    }

    if (_avio_context)
    {
        av_free(_avio_context->buffer);
        avio_context_free(&_avio_context);
    }

    avformat_network_deinit();
}

void Demuxer::open_demuxer(const string &url)
{
    AVDictionary *dictionary = nullptr;

    int ret = 0;
    //媒体打开函数，调用该函数可以获得路径为path的媒体文件的信息，并把这些信息保存到指针ic指向的空间中（调用该函数后会分配一个空间，让指针ic指向该空间）
    if ((ret = avformat_open_input(&_av_format_context, url.c_str(), _input_format, &dictionary)) < -1)
    {
        LOG_ERROR << "avformat_open_input failed! error: " << av_strerror(ret, _err_buf, sizeof (_err_buf) - 1);
        return;
    }

    //调用该函数可以进一步读取一部分视音频数据并且获得一些相关的信息。
    //调用avformat_open_input之后，我们无法获取到正确和所有的媒体参数，所以还得要调用avformat_find_stream_info进一步的去获取。
    if ((ret = avformat_find_stream_info(_av_format_context, nullptr)) < -1)
    {
        LOG_ERROR << "avformat_find_stream_info failed! error: " << av_strerror(ret, _err_buf, sizeof (_err_buf) - 1);
        return;
    }

    //调用avformat_open_input读取到的媒体文件的路径/名字
    LOG_INFO << "媒体文件名称: " << _av_format_context->filename;

    //视音频流的个数，如果一个媒体文件既有音频，又有视频，则nb_streams的值为2。如果媒体文件只有音频，则值为1
    LOG_INFO << "音视频流个数: " << _av_format_context->nb_streams;

    //媒体文件的平均码率,单位为bps
    LOG_INFO << "媒体文件平均码率: " << _av_format_context->bit_rate << " bps";

    LOG_INFO << "duration: " << _av_format_context->duration;

    int64_t tns, thh, tmm, tss;
    tns = (_av_format_context->duration) / AV_TIME_BASE;
    thh = tns / 3600;
    tmm = (tns % 3600) / 60;
    tss = (tns % 60);
    LOG_INFO << "媒体总时长: " << thh << "时:" << tmm << "分:" << tss << "秒";
    // 通过遍历的方式读取媒体文件视频和音频的信息，
    // 新版本的FFmpeg新增加了函数av_find_best_stream，也可以取得同样的效果，但这里为了兼容旧版本还是用这种遍历的方式

    for (int i = 0; i < _av_format_context->nb_streams; ++i)
    {
        AVStream * stream = _av_format_context->streams[i];
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            LOG_INFO << "======================== 音频信息 ============================";
            //如果一个媒体文件既有音频，又有视频，则音频index的值一般为1。但该值不一定准确，所以还是得通过as->codecpar->codec_type判断是视频还是音频
            LOG_INFO << "index: " << stream->index;
            LOG_INFO << "音频采样率: " << float(stream->codecpar->sample_rate / 1000.00f) << "kHz";
            LOG_INFO << "音频采样格式: " << av_get_sample_fmt_name((AVSampleFormat)stream->codecpar->format);
            LOG_INFO << "音频压缩编码格式: " << avcodec_get_name((AVCodecID)stream->codecpar->codec_id);
            LOG_INFO << "音频 duration: " << stream->duration;
            LOG_INFO << "音频 时间基: " << Tools::r2d(stream->time_base);
            LOG_INFO << "音频声道数: " << stream->codecpar->channels;


            double DurationAudio = (double)(stream->duration) * Tools::r2d(stream->time_base); //音频总时长，单位为秒。注意如果把单位放大为毫秒或者微妙，音频总时长跟视频总时长不一定相等的
            LOG_INFO << "音频总时长: "
                     << (int)(DurationAudio / 3600.0) << "时:"
                     << (int)(std::fmod(DurationAudio, 3600.0) / 60) << "分:"
                     << std::fmod(DurationAudio, 60.0) << "秒";
            LOG_INFO << "";
        }
        else if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            LOG_INFO << "======================== 视频信息 ============================";
            LOG_INFO << "index: " << stream->index;
            LOG_INFO << "视频帧率: " << Tools::r2d(stream->avg_frame_rate);
            LOG_INFO << "视频压缩编码格式: " << avcodec_get_name((AVCodecID)stream->codecpar->codec_id);
            LOG_INFO << "帧宽度: " << stream->codecpar->width << " 帧高度: " << stream->codecpar->height;
            LOG_INFO << "视频 duration: " << stream->duration;
            LOG_INFO << "视频 时间基: " << Tools::r2d(stream->time_base);


            double DurationVedio = (double)(stream->duration) * Tools::r2d(stream->time_base); //音频总时长，单位为秒。注意如果把单位放大为毫秒或者微妙，音频总时长跟视频总时长不一定相等的
            LOG_INFO << "视频总时长: "
                     << (int)(DurationVedio / 3600.0) << "时:"
                     << (int) std::fmod(DurationVedio, 3600.0) / 60 << "分:"
                     << std::fmod(DurationVedio, 60.0) << "秒";
        }
    }
}

void Demuxer::open_demuxer_read_frame(const string &url)
{
    AVDictionary *dictionary = nullptr;

    int ret = 0;
    //媒体打开函数，调用该函数可以获得路径为path的媒体文件的信息，并把这些信息保存到指针ic指向的空间中（调用该函数后会分配一个空间，让指针ic指向该空间）
    if ((ret = avformat_open_input(&_av_format_context, url.c_str(), _input_format, &dictionary)) < -1)
    {
        LOG_ERROR << "avformat_open_input failed! error: " << av_strerror(ret, _err_buf, sizeof (_err_buf) - 1);
        return;
    }

    //调用该函数可以进一步读取一部分视音频数据并且获得一些相关的信息。
    //调用avformat_open_input之后，我们无法获取到正确和所有的媒体参数，所以还得要调用avformat_find_stream_info进一步的去获取。
    if ((ret = avformat_find_stream_info(_av_format_context, nullptr)) < -1)
    {
        LOG_ERROR << "avformat_find_stream_info failed! error: " << av_strerror(ret, _err_buf, sizeof (_err_buf) - 1);
        return;
    }

    //调用avformat_open_input读取到的媒体文件的路径/名字
    LOG_INFO << "==================================== av_dump_format begin =======================================";
    av_dump_format(_av_format_context, 0, url.c_str(), 0);
    LOG_INFO << "==================================== av_dump_format end =========================================";
    LOG_INFO << "媒体文件名称: " << _av_format_context->filename;
    //视音频流的个数，如果一个媒体文件既有音频，又有视频，则nb_streams的值为2。如果媒体文件只有音频，则值为1
    LOG_INFO << "音视频流个数: " << _av_format_context->nb_streams;
    //媒体文件的平均码率,单位为bps
    LOG_INFO << "媒体文件平均码率: " << _av_format_context->bit_rate << " bps";
    LOG_INFO << "duration: " << _av_format_context->duration;

    int64_t tns, thh, tmm, tss;
    tns = (_av_format_context->duration) / AV_TIME_BASE;
    thh = tns / 3600;
    tmm = (tns % 3600) / 60;
    tss = (tns % 60);
    LOG_INFO << "媒体总时长: " << thh << "时:" << tmm << "分:" << tss << "秒";
    LOG_INFO << "====================================================================================================";

    int audioindex = av_find_best_stream(_av_format_context, AVMediaType::AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (audioindex < 0)
    {
        LOG_ERROR << "av_find_best_stream failed! error: " << av_strerror(audioindex, _err_buf, sizeof (_err_buf));
        return;
    }

    AVStream * audio_stream = _av_format_context->streams[audioindex];
    LOG_INFO << "======================== 音频信息 ============================";
    //如果一个媒体文件既有音频，又有视频，则音频index的值一般为1。但该值不一定准确，所以还是得通过as->codecpar->codec_type判断是视频还是音频
    LOG_INFO << "index: " << audio_stream->index;
    LOG_INFO << "音频采样率: " << float(audio_stream->codecpar->sample_rate / 1000.00f) << "kHz";
    LOG_INFO << "音频采样格式: " << av_get_sample_fmt_name((AVSampleFormat)audio_stream->codecpar->format);
    LOG_INFO << "音频压缩编码格式: " << avcodec_get_name((AVCodecID)audio_stream->codecpar->codec_id);
    LOG_INFO << "音频 duration: " << audio_stream->duration;
    LOG_INFO << "音频 时间基: " << Tools::r2d(audio_stream->time_base);
    LOG_INFO << "音频声道数: " << audio_stream->codecpar->channels;


    double DurationAudio = (double)(audio_stream->duration) * Tools::r2d(audio_stream->time_base); //音频总时长，单位为秒。注意如果把单位放大为毫秒或者微妙，音频总时长跟视频总时长不一定相等的
    LOG_INFO << "音频总时长: "
             << (int)(DurationAudio / 3600.0) << "时:"
             << (int)(std::fmod(DurationAudio, 3600.0) / 60) << "分:"
             << std::fmod(DurationAudio, 60.0) << "秒";
    LOG_INFO << "";

    int vedioindex = av_find_best_stream(_av_format_context, AVMediaType::AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    AVStream * vedio_stream = _av_format_context->streams[vedioindex];
    LOG_INFO << "======================== 视频信息 ============================";
    LOG_INFO << "index: " << vedio_stream->index;
    LOG_INFO << "视频帧率: " << Tools::r2d(vedio_stream->avg_frame_rate);
    LOG_INFO << "视频压缩编码格式: " << avcodec_get_name((AVCodecID)vedio_stream->codecpar->codec_id);
    LOG_INFO << "帧宽度: " << vedio_stream->codecpar->width << " 帧高度: " << vedio_stream->codecpar->height;
    LOG_INFO << "视频 duration: " << vedio_stream->duration;
    LOG_INFO << "视频 时间基: " << Tools::r2d(vedio_stream->time_base);


    double DurationVedio = (double)(vedio_stream->duration) * Tools::r2d(vedio_stream->time_base); //音频总时长，单位为秒。注意如果把单位放大为毫秒或者微妙，音频总时长跟视频总时长不一定相等的
    LOG_INFO << "视频总时长: "
             << (int)(DurationVedio / 3600.0) << "时:"
             << (int) std::fmod(DurationVedio, 3600.0) / 60 << "分:"
             << std::fmod(DurationVedio, 60.0) << "秒";

    AVPacket *packet = av_packet_alloc();
    int pkt_count = 0;
    int print_max_count = 100;
    while (pkt_count < print_max_count)
    {
        int ret = av_read_frame(_av_format_context, packet);
        if (ret < 0)
        {
            LOG_INFO << "av_read_frame 读取完毕";
            break;
        }

        if (packet->stream_index == audioindex)
        {
            LOG_INFO << "        音频             ";
            LOG_WARNING << "audio dts: " << packet->dts;
            LOG_WARNING << "audio pts: " << packet->pts;
            LOG_WARNING << "audio size: " << packet->size;
            LOG_WARNING << "audio pos: " << packet->pos;
            LOG_WARNING << "audio duration: " << packet->duration * av_q2d(_av_format_context->streams[audioindex]->time_base);
        }
        else if (packet->stream_index == vedioindex)
        {
            LOG_INFO << "        视频             ";
            LOG_WARNING << "vedio dts: " << packet->dts;
            LOG_WARNING << "vedio pts: " << packet->pts;
            LOG_WARNING << "vedio size: " << packet->size;
            LOG_WARNING << "vedio pos: " << packet->pos;
            LOG_WARNING << "vedio duration: " << packet->duration * av_q2d(_av_format_context->streams[vedioindex]->time_base);
        }

        LOG_INFO << "\n";
        pkt_count++;
        av_packet_unref(packet);
    }

    if(packet)
        av_packet_free(&packet);
}
