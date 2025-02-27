cmake_minimum_required(VERSION 3.10)

set(ITEM_NAME "all_example")

project(${ITEM_NAME})

set(CMAKE_CXX_STANDARD 14)

# 设置 FFmpeg 路径
set(FFMPEG_DIR "E:/DevelopSoftware/MSYS2/home/loken/ffmpeg/build64/ffmpeg-n4.4.1-mingw")

# 添加头文件路径
include_directories(${FFMPEG_DIR}/include)

# 添加库文件路径
link_directories(${FFMPEG_DIR}/lib)

set(OUTPUT_DIR ${PROJECT_SOURCE_DIR}/../bin)
file(MAKE_DIRECTORY ${OUTPUT_DIR})
# 设置可执行文件输出路径
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR})
add_executable(${ITEM_NAME} main.cpp
        av_util_example.cpp
        av_util_example.h
        global.h
        av_logger.cpp
        av_logger.h
        avio_file.cpp
        avio_file.h
        avio_reading.cpp
        avio_reading.h
        demuxer.cpp
        demuxer.h
        remuxer.cpp
        remuxer.h
        encode_video.cpp
        encode_video.h
        demuxing_decoding.cpp
        demuxing_decoding.h)

# 添加编译选项
if(MINGW)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
endif()

# 链接库，注意顺序很重要
target_link_libraries(${ITEM_NAME}
        ${FFMPEG_DIR}/lib/libavformat.dll.a
        ${FFMPEG_DIR}/lib/libavcodec.dll.a
        ${FFMPEG_DIR}/lib/libavutil.dll.a
        ${FFMPEG_DIR}/lib/libswscale.dll.a
        ${FFMPEG_DIR}/lib/libswresample.dll.a
        ${FFMPEG_DIR}/lib/libavdevice.dll.a
        ${FFMPEG_DIR}/lib/libavfilter.dll.a
        ${FFMPEG_DIR}/lib/libpostproc.dll.a
)