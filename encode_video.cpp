//
// Created by HWK15 on 2025/2/24.
//

#include "encode_video.h"

void EncodeVideo::start_encode(const string &filename, const string &codec_name)
{
    int ret = 0;
    // 查找指定名称的编码器 (mpeg1video)
    _codec = avcodec_find_encoder_by_name(codec_name.c_str());
    if (!_codec)
    {
        LOG_ERROR << "avcodec_find_decoder_by_name failed codec_name：" << codec_name;
        return;
    }

    // 通过解码器分配上下文
    _codec_context = avcodec_alloc_context3(_codec);
    if (!_codec_context)
    {
        LOG_ERROR << "avcodec_alloc_context3 failed";
        return;
    }

    /* 设置编码参数 */
    _codec_context->bit_rate = 400000;  // 设置比特率，单位 bps

    _codec_context->width = 1920; // 视频宽度（必须是偶数）
    _codec_context->height = 1080;  // 视频高度（必须是偶数）

    _codec_context->time_base = (AVRational){1, 25}; // 帧时间基数 (1/25秒)
    _codec_context->framerate = (AVRational){25, 1}; // 帧率：25 fps

    _codec_context->gop_size = 10; // 设置 GOP（关键帧间隔）：每 10 帧一个关键帧
    _codec_context->max_b_frames = 1;    // 允许最多 1 个 B 帧（双向预测帧）
    _codec_context->pix_fmt = AV_PIX_FMT_YUV420P; // 选择 YUV420P 颜色格式

    ///< 如果是 H.264，设置编码预设
    if (_codec->id == AVCodecID::AV_CODEC_ID_H264)
    {
        // H.264 设置慢速编码，提升压缩率
        av_opt_set(_codec_context->priv_data, "preset", "slow", 0);
    }

    // 打开编码器
    ret = avcodec_open2(_codec_context, _codec, nullptr);
    if (ret < 0)
    {
        LOG_ERROR << "avcodec_open2 failed" << av_err2str(ret);
        return;
    }

    // 打开文件
    _ofstream.open(filename, ios::binary);
    if (!_ofstream.is_open())
    {
        LOG_ERROR << "文件打开失败 filename: " << filename;
        return;
    }


    AVPacket* pkt = av_packet_alloc();
    if (!pkt)
    {
        LOG_ERROR << "av_packet_alloc failed";
        return;
    }

     AVFrame* frame = av_frame_alloc();
    if (!frame)
    {
        LOG_ERROR << "av_frame_alloc failed";
        return;
    }

    frame->width = _codec_context->width;
    frame->height = _codec_context->height;
    frame->format = _codec_context->pix_fmt;

    ret = av_frame_get_buffer(frame, 0);  // 分配帧数据缓冲区
    if (ret < 0)
    {
        LOG_ERROR << "Could not allocate the video frame data";
        return;
    }

    /* 逐帧编码 */
    /* encode 10 second of video */
    int y = 0, x = 0;
    for (int i = 0; i < 250; ++i)
    {
        // 确保帧可写
        ret = av_frame_is_writable(frame);
        if (ret < 0)
            return;

        /* prepare a dummy image */
        /* Y */
        for (y = 0; y < _codec_context->height; ++y)
        {
            for (x = 0; x < _codec_context->width; ++x)
            {
                // 0 代表 Y，第二个索引代表该值在内存的位置
                // frame->linesize[0] 代表Y这一行的字节大小
                frame->data[0][y * frame->linesize[0] + x] = x + y + i * 3;
            }
        }

        /* Cb and Cr */
        for (y = 0; y < _codec_context->height / 2; y++)
        {
            for (x = 0; x < _codec_context->width / 2; x++)
            {
                frame->data[1][y * frame->linesize[1] + x] = 128 + y + i * 2;
                frame->data[2][y * frame->linesize[2] + x] = 64 + x + i * 5;
            }
        }

        frame->pts = i;

        /* encode the image */
        encode(frame, pkt);
    }

    /* flush the encoder */
    encode(NULL, pkt);

    // MPEG1/MPEG2 视频流结束标志（0x000001B7）
    uint8_t endcode[] = { 0, 0, 1, 0xb7 };
    /* add sequence end code to have a real MPEG file */
    if (_codec->id == AV_CODEC_ID_MPEG1VIDEO || _codec->id == AV_CODEC_ID_MPEG2VIDEO)
        _ofstream.write((char *)endcode, sizeof (endcode));

    av_frame_free(&frame);
    av_packet_free(&pkt);
}

void EncodeVideo::encode(AVFrame *frame, AVPacket *pkt)
{
    if (!_codec_context || !frame || !pkt)
        return;

    int ret;

    if (frame)
        LOG_INFO << "Send frame " << frame->pts;

    ret = avcodec_send_frame(_codec_context, frame);
    if (ret < 0)
    {
        LOG_ERROR << "Error sending a frame for encoding error: " << av_err2str(ret);
        return;
    }

    while (ret >= 0)
    {
        ret = avcodec_receive_packet(_codec_context, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret < 0)
        {
            LOG_ERROR << "Error during encoding";
            return;
        }

        LOG_INFO << "Write packet " << frame->pts << " size=" << pkt->size;
        _ofstream.write((char *)pkt->data, pkt->size);
        av_packet_unref(pkt);
    }
}

EncodeVideo::EncodeVideo()
{

}

EncodeVideo::~EncodeVideo()
{
    _ofstream.close();
    avcodec_free_context(&_codec_context);
}
