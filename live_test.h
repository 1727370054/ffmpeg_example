/********************************************************************************
* @author: hwk
* @email: orionlink@163.com
* @date: 2025/3/1 18:08
* @version: 1.0
* @description: 
********************************************************************************/

#ifndef ALL_EXAMPLE_LIVE_TEST_H
#define ALL_EXAMPLE_LIVE_TEST_H

#include "global.h"

class LiveTest
{
public:
    LiveTest();
    ~LiveTest();
    void start_live(const string& in_filename, const string& out_url);

private:
    unique_ptr<AVFormatContext, AVFormatContextDeleter> _ifmt_ctx;
    unique_ptr<AVFormatContext, AVFormatContextDeleter> _ofmt_ctx;
    unique_ptr<AVPacket, AVPacketDeleter> _packet;
    AVOutputFormat *_out_fmt = nullptr;
};


#endif //ALL_EXAMPLE_LIVE_TEST_H
