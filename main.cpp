#include <iostream>

#include "av_util_example.h"
#include "av_logger.h"
#include "avio_file.h"
#include "avio_reading.h"
#include "demuxer.h"
#include "remuxer.h"
#include "encode_video.h"
#include "demuxing_decoding.h"
#include "filtering_video.h"
#include "live_test.h"

#define UTIL_TEST 0
#define CUSTOM_LOG 0 // 自定义日志

int main(int argc, char *argv[])
{
#if UTIL_TEST
    AVUtilExample example;
    example.test_log();
    example.test_dictionary();
    example.test_parse_util();
#endif

#if CUSTOM_LOG
    std::string str = "我是字符串";
    LOG_DEBUG << "DEBUG 级别 " << 64;
    LOG_INFO << "INFO 级别 " << str;
    LOG_WARNING << "WARNING 级别 ";
    LOG_ERROR << "ERROR 级别 ";
#endif

    std::string url = "";
    std::string out_filename = "";
    if (argc >= 2)
    {
        url = argv[1];
    }

    if (argc >= 3)
    {
        out_filename = argv[2];
    }

//    AVIOFile file;
//    file.open("juren-h265.mp4");
//    file.open_custom_avio("juren-h265.mp4");

//    AVIOReading reading;
//    reading.open("juren-h265.mp4");

//    Demuxer demuxer;
//    demuxer.open_demuxer(url);
//    demuxer.open_demuxer_read_frame(url);

//    Remuxer remuxer;
//    remuxer.start_remuxer(url.c_str(), out_filename.c_str());

//    EncodeVideo encode_video;
//    encode_video.start_encode("encode_test.mp4", "mpeg1video");

    MediaDecoder media_decoder("22-28.mp4", "22-28.yuv", "22-28.pcm");
    media_decoder.decode();

//    FilteringVideo filtering_video;
//    string filter_descr = "scale=640:480,hflip";
//    if (argc >= 4)
//    {
//        filter_descr = argv[3];
//    }
//    filtering_video.start_filtering(url, out_filename, filter_descr);

//    LiveTest live_test;
//    out_filename = "rtmp://127.0.0.1/live/channal1";
//    live_test.start_live(url, out_filename);

    return 0;
}
