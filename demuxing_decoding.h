//
// Created by HWK15 on 2025/2/25.
//

#ifndef ALL_EXAMPLE_DEMUXING_DECODING_H
#define ALL_EXAMPLE_DEMUXING_DECODING_H

#include "global.h"

class VideoDecoder;
class AudioDecoder;

///< 媒体解码器基类
class MediaDecoder
{
public:
    explicit MediaDecoder(const string &src_filename, const std::string& video_file,
                          const std::string& audio_file);
    ~MediaDecoder() {}

    void decode();
protected:
    void init();

    void processPacket(const AVPacket* pkt);

    void flushDecoders();

    unique_ptr<AVFormatContext, AVFormatContextDeleter> _format_context;
    unique_ptr<AVFrame, AVFrameDeleter> _frame;
    unique_ptr<AVPacket, AVPacketDeleter> _packet;
    string _src_filename;

    std::unique_ptr<VideoDecoder> videoDecoder;
    std::unique_ptr<AudioDecoder> audioDecoder;
};

class Decoder
{
public:
    Decoder(AVFormatContext* format_ctx) : _format_ctx(format_ctx) {}
    virtual ~Decoder(){}
    virtual int sendPacket(const AVPacket* pkt) = 0;
    virtual int receiveFrame(AVFrame* frame) = 0;

    int getStreamIndex() const { return _stream_index; }
protected:
    /**
     * 初始化解码器上下文
     */
    virtual void initCodecContext() = 0;

    /**
     * 处理帧数据
     * @param frame 一帧
     * @return
     */
    virtual int processFrame(AVFrame* frame) = 0;

    unique_ptr<AVCodecContext, AVCodecContextDeleter> _codec_context;
    AVStream* _stream{nullptr};
    int _stream_index{-1};
    AVFormatContext* _format_ctx;
    int _frame_count{0};
};

class VideoDecoder : public Decoder
{
public:
    explicit VideoDecoder(AVFormatContext* format_ctx, const string& dst_filename);
    ~VideoDecoder() override;

    int sendPacket(const AVPacket* pkt) override;

    int receiveFrame(AVFrame* frame) override;

    void cleanup();

    // 获取播放命令
    std::string getPlayCommand() const
    {
        return "ffplay -f rawvideo -pix_fmt " +
               std::string(av_get_pix_fmt_name(_pix_fmt)) +
               " -video_size " + std::to_string(_width) + "x" + std::to_string(_pix_fmt) +
               " " + _dst_filename;
    }
private:
    void initCodecContext() override;

    int processFrame(AVFrame* frame) override;

    int _width{0}, _height{0};
    AVPixelFormat _pix_fmt;

    ofstream _ofstream;
    string _dst_filename;

    ///< 视频帧缓冲区
    /**
     * _dest_data 每个元素指向一个颜色分量的起始地址。destData[0] 指向 Y（亮度） 数据。
     * destData[1] 指向 U（Cb色度） 数据。destData[2] 指向 V（Cr色度） 数据。
     */
    uint8_t* _dest_data[4] = {nullptr};
    int _dest_line_size[4]{0};         ///< 记录每个 plane 的 步长（即每行数据占用的字节数）。
    int _dest_buf_size{0};             ///< 视频帧缓冲区大小
};

class AudioDecoder  : public Decoder
{
public:
    AudioDecoder(AVFormatContext* format_ctx, const string& dst_filename);
    ~AudioDecoder() override { cleanup(); }

    int sendPacket(const AVPacket* pkt) override;

    int receiveFrame(AVFrame* frame) override;

    void cleanup();

    // 从采样格式获取格式字符串
    std::string getSampleFormatString(enum AVSampleFormat sampleFmt) const;

    // 获取播放命令
    std::string getPlayCommand() const;
private:
    void initCodecContext() override;

    int processFrame(AVFrame* frame) override;

    string _dst_filename;
    ofstream _ofstream;
};

#endif //ALL_EXAMPLE_DEMUXING_DECODING_H
