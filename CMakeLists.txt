cmake_minimum_required(VERSION 3.5.0)
project(FFMPEG_AUDIO_DECODING VERSION 0.1.0 LANGUAGES C CXX)
add_executable(FFMPEG_AUDIO_DECODING main.cpp)

target_link_libraries(FFMPEG_AUDIO_DECODING avformat avcodec avutil swresample)