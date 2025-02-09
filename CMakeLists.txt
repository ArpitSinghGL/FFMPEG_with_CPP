cmake_minimum_required(VERSION 3.5.0)
project(FFMPEG_AUDIO_DECODING VERSION 0.1.0 LANGUAGES C CXX)


find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED libavformat libavcodec libavutil libswresample libswscale)

include_directories(${FFMPEG_INCLUDE_DIRS})
link_directories(${FFMPEG_LIBRARY_DIRS})
add_definitions(${FFMPEG_CFLAGS_OTHER})

# Use the same name for the executable as the project
add_executable(${PROJECT_NAME} frame_to_image.cpp)
target_link_libraries(${PROJECT_NAME} ${FFMPEG_LIBRARIES})
