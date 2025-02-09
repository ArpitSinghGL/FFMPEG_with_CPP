#include <iostream>
#include <stdio.h>

extern "C"
{
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libavutil/avutil.h>
    #include <libavutil/frame.h>
    #include <libavutil/pixdesc.h>
    #include <libavutil/pixelutils.h>
    #include <libavutil/pixfmt.h>
    #include <libavutil/imgutils.h>
    #include <libswscale/swscale.h>
    #include <inttypes.h>
}

void print_yuv420_pixel_data_y_plane(AVFrame* frame)
{
    int height = frame -> height;
    int width = frame -> width;

    printf("Printing the Y plane pixel data: \n");

    for(int y = 0 ; y < height ; y++)
    {
        uint8_t* row = (frame -> data)[0] + y * (frame -> linesize)[0];

        for(int x = 0 ; x < width ; x++)
        {
            printf("%u " , row[x]);
        }

        printf("\n");
    }
}

void saveFrameAsPPM(AVFrame *frame, int width, int height, const char *frame_image_name) 
{
    FILE *file = fopen(frame_image_name, "wb");
    if (!file) {
        fprintf(stderr, "Could not open file for writing\n");
        return;
    }

    // Write PPM header
    fprintf(file, "P6\n%d %d\n%d\n", width, height, 255);

    // Write pixel data (RGB)

    for (int y = 0; y < height; y++) {
        fwrite(frame->data[0] + y * frame->linesize[0], 1, width * 3, file);
    }

    printf("Width, Height : %d, %d\n\n", width, height);  

    fclose(file);
}


int main(int argc, char* argv[])
{
    int returnValue = 0;

    // Open the file using libavformat

    AVFormatContext* av_format_context = avformat_alloc_context();

    if(!av_format_context)
    {
        printf("Could not create AVFormatContext !\n");
        return 1;
    }

    char* input_filename = argv[1];

    returnValue = avformat_open_input(&av_format_context , input_filename , NULL , NULL);

    if(returnValue < 0)
    {
        printf("Could not open input file !\n");
        return 2;
    }

    returnValue = avformat_find_stream_info(av_format_context , NULL);

    if(returnValue < 0)
    {
        printf("Could not find stream info !\n");
        return 3;
    }

    // Finding the valid Video and Audio streams from the input file

    unsigned int number_of_streams = av_format_context -> nb_streams;

    int video_stream = -1 , audio_stream = -1;

    printf("The number of streams in the input file: %u\n" , number_of_streams);

    for(int stream = 0 ; stream < number_of_streams ; stream++)
    {
        printf("stream %d ---> AVMediaType: %d\n" , stream , av_format_context -> streams[stream] -> codecpar -> codec_type);
        if(av_format_context -> streams[stream] -> codecpar -> codec_type == AVMEDIA_TYPE_VIDEO)
        {
            video_stream = stream;
        }
        if(av_format_context -> streams[stream] -> codecpar -> codec_type == AVMEDIA_TYPE_AUDIO)
        {
            audio_stream = stream;
        }
    }

    if(video_stream == -1)
    {
        printf("Could not find video stream\n");
        return -1;
    }

    // if(audio_stream == -1)
    // {
    //     printf("Could not find audio stream\n");
    //     return -1;
    // }

    // Get codec parameters and codec context
    AVCodecParameters* video_codec_parameters = av_format_context -> streams[video_stream] -> codecpar;

    const AVCodec* video_codec = avcodec_find_decoder(video_codec_parameters -> codec_id);
    printf("Video Codec ID: %d\n" , video_codec -> id);

    const AVCodec* video_encoder = avcodec_find_encoder(video_codec_parameters -> codec_id);
    const AVCodec* video_decoder = avcodec_find_decoder(video_codec_parameters -> codec_id);

    printf("Name of Video Encoder: %s\n" , video_encoder -> long_name);
    printf("Name of Video Decoder: %s\n" , video_decoder -> long_name);
    printf("Video BitRate: %ld\n" , video_codec_parameters -> bit_rate);
    printf("Video Resolution: %d x %d\n" , video_codec_parameters -> width , video_codec_parameters -> height);

    // AVCodecParameters* audio_codec_parameters = av_format_context -> streams[audio_stream] -> codecpar;

    // const AVCodec* audio_encoder = avcodec_find_encoder(audio_codec_parameters -> codec_id);
    // const AVCodec* audio_decoder = avcodec_find_decoder(audio_codec_parameters -> codec_id);

    // printf("Name of Audio Encoder: %s\n" , audio_encoder -> long_name);
    // printf("Name of Audio Decoder: %s\n" , audio_decoder -> long_name);
    // printf("Audio BitRate: %ld\n" , audio_codec_parameters -> bit_rate);
    // printf("Audio Sample Rate: %d\n" , audio_codec_parameters ->sample_rate);
    // printf("Audio Frame Size: %d\n" , audio_codec_parameters -> frame_size);
    // printf("Number of Audio Channels: %d\n" , audio_codec_parameters -> channels);

    // Set up a codec context
    AVCodecContext* video_decoder_context = avcodec_alloc_context3(video_decoder);

    if(!video_decoder_context)
    {
        printf("Could not create AVvideo_decoder_context\n");
        return 6;
    }

    if(avcodec_parameters_to_context(video_decoder_context , video_codec_parameters) < 0)
    {
        printf("Could not initialize AVvideo_decoder_context\n");
        return -1;
    }

    if(avcodec_open2(video_decoder_context , video_decoder , NULL) < 0)
    {
        printf("Could not open codec\n");
        return -1;
    }

    // Media Playback Pipeline:
    // Input file -> DEMUXER -> AVPackets(Compressed Data) -> DECODER -> AVFrame(Raw Data)

    AVFrame* av_frame = av_frame_alloc();

    if(!av_frame)
    {
        printf("Could not allocate input video frame\n");
        return -1;
    }

    AVPacket* av_packet = av_packet_alloc();

    if(!av_packet)
    {
        printf("Could not allocate AVPacket\n");
        return -1;
    }


    // ----------- CONVERSION FROM YUV420P pixel format to RGB24 pixel format -----------------

    struct SwsContext *swsCtx = sws_getContext
    (
    video_decoder_context->width, 
    video_decoder_context->height, 
    video_decoder_context->pix_fmt,  // Source (YUV420P)
    video_decoder_context->width, 
    video_decoder_context->height, 
    AV_PIX_FMT_RGB24,       // Destination (RGB24)
    SWS_BILINEAR, 
    NULL, 
    NULL, 
    NULL
    );

    AVFrame *frameRGB = av_frame_alloc();

    if(frameRGB == NULL)
    {
        printf("Could not allocate output video frame\n");
        return -1;
    }

    uint8_t *buffer = nullptr;
    
    // Calculate required buffer size and allocate

    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, video_decoder_context->width, video_decoder_context->height, 1);
    
    buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));
    
    // Associate buffer with the RGB frame
    av_image_fill_arrays(frameRGB->data, frameRGB->linesize, buffer, AV_PIX_FMT_RGB24, video_decoder_context->width, video_decoder_context->height, 1);

    int frame_count = 0;

    while(av_read_frame(av_format_context , av_packet) >= 0)
    {
        if(av_packet -> stream_index == video_stream)
        {
            // Decode the Video Packet(AVPacket: Compressed Video Data)
            // to get Video Frames (AVFrame: Raw Video Data)

            // Note: In case of Video, the Packet contains only one Frame.

            // Sending the Video Packet to the decoder to get Video Frames
            returnValue = avcodec_send_packet(video_decoder_context , av_packet);
            if(returnValue < 0)
            {
                printf("Error sending packet for decoding\n");
                break;
            }
            returnValue = avcodec_receive_frame(video_decoder_context , av_frame);
            if(returnValue == AVERROR(EAGAIN) || returnValue == AVERROR_EOF)
            {
                printf("Some error occurred\n");
                continue;
            }
            else if(returnValue < 0)
            {
                printf("Failed to decode packet\n");
            }
            else
            {
                // Printing Basic Frame Properties
                printf("Frame Number: %d\n" , video_decoder_context -> frame_number);
                printf("Type: %c\n" , av_get_picture_type_char(av_frame -> pict_type));
                printf("Size: %d bytes\n" , av_frame -> pkt_size);
                printf("pts: %ld\n" , av_frame -> pts);
                printf("key_frame: %d\n" , av_frame -> key_frame);
                printf("Height: %d\n" , video_decoder_context -> height);
                printf("Width: %d\n" , video_decoder_context -> width);
                printf("linesize[0]: %d bytes\n" , (av_frame -> linesize)[0]);
                printf("linesize[1]: %d bytes\n" , (av_frame -> linesize)[1]);
                printf("linesize[2]: %d bytes\n" , (av_frame -> linesize)[2]);

                enum AVPixelFormat pixel_format = video_decoder_context -> pix_fmt;

                const char* pixel_format_name = av_get_pix_fmt_name(pixel_format);

                printf("Pixel Format: %s\n" , pixel_format_name);

                // print_yuv420_pixel_data_y_plane(av_frame);
                
                // Convert image from YUV420P to RGB24
                sws_scale(swsCtx, av_frame->data, av_frame->linesize, 0, video_decoder_context->height, frameRGB->data, frameRGB->linesize);
          
                // Save the RGB frame as an image
                char frame_image_name[32];

                sprintf(frame_image_name, "frame%d.ppm", video_decoder_context -> frame_number++);

                saveFrameAsPPM(frameRGB, av_frame->width , av_frame->height, frame_image_name);

                frame_count += 1;

                if(frame_count > 20)
                {
                    av_packet_unref(av_packet);
                    break;
                }
            }
        }
        av_packet_unref(av_packet);
    }

    sws_freeContext(swsCtx);

    // Free the YUV frame
    av_frame_free(&av_frame);

    // Free the RGB frame
    av_frame_free(&frameRGB);

    av_packet_free(&av_packet);

    avformat_free_context(av_format_context);

    avcodec_free_context(&video_decoder_context);

    avformat_close_input(&av_format_context);

    return 0;
}