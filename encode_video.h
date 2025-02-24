//
// Created by HWK15 on 2025/2/24.
//

#ifndef ALL_EXAMPLE_ENCODE_VIDEO_H
#define ALL_EXAMPLE_ENCODE_VIDEO_H

#include "global.h"

class EncodeVideo
{
public:
    EncodeVideo();
    ~EncodeVideo();

    void start_encode(const std::string& filename, const std::string& codec_name);
private:
    void encode(AVFrame *frame, AVPacket *pkt);

    AVCodecContext* _codec_context = nullptr;
    AVCodec* _codec = nullptr;
    std::ofstream _ofstream;
};


#endif //ALL_EXAMPLE_ENCODE_VIDEO_H
