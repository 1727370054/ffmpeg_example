//
// Created by HWK15 on 2025/2/22.
//

#include "avio_file.h"

bool AVIOFile::open(const string &url)
{
    _av_format_context = avformat_alloc_context();
    if (!_av_format_context)
    {
        LOG_ERROR << "avformat_alloc_context 分配失败";
        return false;
    }

    LOG_INFO << "avformat_alloc_context 分配成功";
    _av_format_context->flags |= AVFMT_FLAG_NOBUFFER;  // 减少一些缓冲相关的日志

    int res = avformat_open_input(&_av_format_context, url.c_str(), _input_format, nullptr);
    if (res < 0)
    {
        LOG_ERROR << "avformat_open_input 失败";
        return false;
    }

    LOG_INFO << "avformat_open_input 成功";

    if (avformat_find_stream_info(_av_format_context, nullptr) < 0)
    {
        LOG_ERROR << "avformat_find_stream_info 失败";
        return false;
    }

    LOG_INFO << "avformat_find_stream_info 成功";

    return true;
}

AVIOFile::AVIOFile()
{
    avformat_network_init();
    LOG_INFO << "avformat_network 初始化成功";
}

AVIOFile::~AVIOFile()
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

    if (_ifstream.is_open())
    {
        _ifstream.close();
    }
}

bool AVIOFile::open_custom_avio(const string &url)
{
    _ifstream.open(url, ios::in | ios::binary);
    if (!_ifstream.is_open())
    {
        LOG_ERROR << "打开文件失败: " << url;
        return false;
    }

    // Allocate buffer for AVIO
    int buffer_size = 32768;  // Increased buffer size for better performance
//    int buffer_size = 1024;  // Increased buffer size for better performance
    unsigned char* buffer = (unsigned char*)av_malloc(buffer_size);
    if (!buffer)
    {
        LOG_ERROR << "av_malloc 失败: " << url;
        return false;
    }

    // Create AVIO context
    _avio_context = avio_alloc_context(buffer, buffer_size, 0, this,
                                       &AVIOFile::read_packet, nullptr, &AVIOFile::seek_packet);
    if (!_avio_context)
    {
        LOG_ERROR << "avio_alloc_context 失败";
        av_free(buffer);
        return false;
    }

    // Allocate format context
    _av_format_context = avformat_alloc_context();
    if (!_av_format_context)
    {
        LOG_ERROR << "avformat_alloc_context 分配失败";
        av_free(_avio_context->buffer);
        avio_context_free(&_avio_context);
        return false;
    }

    // Set custom IO
    _av_format_context->pb = _avio_context;
    _av_format_context->flags |= AVFMT_FLAG_CUSTOM_IO;
    _av_format_context->flags |= AVFMT_FLAG_NOBUFFER;  // 减少一些缓冲相关的日志

    // Open input
    int res = avformat_open_input(&_av_format_context, nullptr, _input_format, nullptr);
    if (res < 0)
    {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(res, errbuf, AV_ERROR_MAX_STRING_SIZE);
        LOG_ERROR << "avformat_open_input 失败: " << errbuf;

        av_free(_avio_context->buffer);
        avio_context_free(&_avio_context);
        return false;
    }

    LOG_INFO << "avformat_open_input 成功";

    // Find stream info
    if (avformat_find_stream_info(_av_format_context, nullptr) < 0)
    {
        LOG_ERROR << "avformat_find_stream_info 失败";
        avformat_close_input(&_av_format_context);
        av_free(_avio_context->buffer);
        avio_context_free(&_avio_context);
        return false;
    }

    LOG_INFO << "Custom AVIO successfully opened";
    av_dump_format(_av_format_context, 0, url.c_str(), 0);
    return true;
}

int AVIOFile::read_packet(void *opaque, uint8_t *buf, int buf_size)
{
    static int call_count = 0;
    AVIOFile *self = static_cast<AVIOFile*>(opaque);
    if (!self) return 0;

    self->_ifstream.read((char*)buf, buf_size);

    int bytes_read = self->_ifstream.gcount();
    LOG_DEBUG << "读取了: " << bytes_read;

    if (bytes_read > 0)
    {
        self->_total_bytes_read += bytes_read;
    }

    call_count++;
    LOG_INFO << "======================================================";
    LOG_INFO << "第 " << call_count << " 次调用 read_packet";
    LOG_INFO << "本次读取: " << bytes_read << " 字节";
    LOG_INFO << "累计读取: " << self->_total_bytes_read << " 字节";

    return bytes_read;
}

int AVIOFile::write_packet(void *opaque, uint8_t *buf, int buf_size)
{
    return 0;
}

int64_t AVIOFile::seek_packet(void *opaque, int64_t offset, int whence)
{
    AVIOFile *self = static_cast<AVIOFile*>(opaque);
    if (!self) return 0;
    if (whence == AVSEEK_SIZE) return -1;

    static int seek_count = 0;
    seek_count++;
    LOG_INFO << "======================================================";
    LOG_INFO << "第 " << seek_count << " 次调用 seek_packet";
    LOG_INFO << "seek to offset: " << offset << ", whence: " << whence;

    ios_base::seekdir dir;
    switch (whence)
    {
        case SEEK_SET:
            dir = ios::beg;
            break;
        case SEEK_CUR:
            dir = ios::cur;
            break;
        case SEEK_END:
            dir = ios::end;
            break;
    }

    self->_ifstream.seekg(offset, dir);

    return self->_ifstream.tellg();
}
