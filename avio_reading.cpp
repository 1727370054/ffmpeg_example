//
// Created by HWK15 on 2025/2/23.
//

#include "avio_reading.h"

bool AVIOReading::open(const string &url)
{
    uint8_t  *avio_ctx_buffer = nullptr;
    size_t avio_ctx_buffer_size = 32768;

    if (av_file_map(url.c_str(), &_buffer_data.ptr, &_buffer_data.size, 0 , nullptr) < -1)
    {
        LOG_ERROR << "av_file_map error";
        return false;
    }

    _buffer_data.original_ptr = _buffer_data.ptr;
    _buffer_data.original_size = _buffer_data.size;

    avio_ctx_buffer = (uint8_t*)av_malloc(avio_ctx_buffer_size);
    if (!avio_ctx_buffer)
    {
        LOG_ERROR << "av_malloc 失败: " << url;
        return false;
    }

    // Create AVIO context
    _avio_context = avio_alloc_context(avio_ctx_buffer, avio_ctx_buffer_size, 0, this,
                                       &AVIOReading::read_packet, 0, &AVIOReading::seek_packet);
    if (!_avio_context)
    {
        LOG_ERROR << "avio_alloc_context 失败";
        av_free(avio_ctx_buffer);
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

    if (avformat_open_input(&_av_format_context, "", _input_format, nullptr) < 0)
    {
        LOG_ERROR << "avformat_find_stream_info 失败";
        av_free(_avio_context->buffer);
        avio_context_free(&_avio_context);
        return false;
    }

    if (avformat_find_stream_info(_av_format_context, nullptr) < 0)
    {
        LOG_ERROR << "avformat_find_stream_info 失败";
        av_free(_avio_context->buffer);
        avio_context_free(&_avio_context);
        return false;
    }

    av_dump_format(_av_format_context, 0, url.c_str(), 0);
    return true;
}

int AVIOReading::read_packet(void *opaque, uint8_t *buf, int buf_size)
{
    static int call_count = 0;
    static int total_bytes_read = 0;
    AVIOReading *self = static_cast<AVIOReading*>(opaque);
    if (!self) return 0;

    buf_size = FFMIN(buf_size, self->_buffer_data.size); // 防止缓冲区溢出

    memcpy(buf, self->_buffer_data.ptr, buf_size);
    self->_buffer_data.ptr += buf_size;
    self->_buffer_data.size -= buf_size;

    call_count++;
    total_bytes_read += buf_size;

    LOG_INFO << "======================================================";
    LOG_INFO << "第 " << call_count << " 次调用 read_packet";
    LOG_INFO << "本次读取: " << buf_size << " 字节";
    LOG_INFO << "累计读取: " << total_bytes_read << " 字节";

    return buf_size;
}

int AVIOReading::write_packet(void *opaque, uint8_t *buf, int buf_size)
{
    return 0;
}

int64_t AVIOReading::seek_packet(void *opaque, int64_t offset, int whence)
{
    AVIOReading *self = static_cast<AVIOReading*>(opaque);
    if (!self) return -1;

    // 保存原始文件大小用于边界检查
    int64_t original_size = self->_buffer_data.original_size;
    uint8_t* original_ptr = self->_buffer_data.original_ptr;

    static int seek_count = 0;
    seek_count++;
    LOG_INFO << "======================================================";
    LOG_INFO << "第 " << seek_count << " 次调用 seek_packet";
    LOG_INFO << "seek to offset: " << offset << ", whence: " << whence;

    switch (whence)
    {
        case SEEK_SET:
        {
            if (offset < 0 || offset > original_size)
                return -1;
            self->_buffer_data.ptr = original_ptr + offset;
            self->_buffer_data.size = original_size - offset;
        }
            break;
        case SEEK_CUR:
        {
            int64_t current_pos = self->_buffer_data.ptr - original_ptr;
            int64_t new_pos = current_pos + offset;
            if (new_pos < 0 || new_pos > original_size) return -1;

            self->_buffer_data.ptr = original_ptr + new_pos;
            self->_buffer_data.size = original_size - new_pos;
        }
            break;
        case SEEK_END:
        {
            if (offset > 0 || -offset > original_size)
                return -1;

            self->_buffer_data.ptr = original_ptr + (original_size + offset);
            self->_buffer_data.size = -offset;
        }
            break;
        case AVSEEK_SIZE: // 返回文件大小
            return original_size;
        default:
            return -1;
    }

    return 0;
}
