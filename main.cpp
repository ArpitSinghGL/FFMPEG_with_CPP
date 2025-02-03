#include <iostream>

extern "C"
{
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <inttypes.h>
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

    char* filename = argv[1];

    returnValue = avformat_open_input(&av_format_context , filename , NULL , NULL);

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
        return 4;
    }

    if(audio_stream == -1)
    {
        printf("Could not find audio stream\n");
        return 5;
    }

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

    AVCodecParameters* audio_codec_parameters = av_format_context -> streams[audio_stream] -> codecpar;

    const AVCodec* audio_encoder = avcodec_find_encoder(audio_codec_parameters -> codec_id);
    const AVCodec* audio_decoder = avcodec_find_decoder(audio_codec_parameters -> codec_id);

    printf("Name of Audio Encoder: %s\n" , audio_encoder -> long_name);
    printf("Name of Audio Decoder: %s\n" , audio_decoder -> long_name);
    printf("Audio BitRate: %ld\n" , audio_codec_parameters -> bit_rate);
    printf("Audio Sample Rate: %d\n" , audio_codec_parameters ->sample_rate);
    printf("Audio Frame Size: %d\n" , audio_codec_parameters -> frame_size);
    printf("Number of Audio Channels: %d\n" , audio_codec_parameters -> channels);

    // Set up a codec context
    AVCodecContext* video_codec_context = avcodec_alloc_context3(video_codec);

    if(!video_codec_context)
    {
        printf("Could not create AVCodecContext\n");
        return 6;
    }

    if(avcodec_parameters_to_context(video_codec_context , video_codec_parameters) < 0)
    {
        printf("Could not initialize AVCodecContext\n");
        return 7;
    }

    if(avcodec_open2(video_codec_context , video_codec , NULL) < 0)
    {
        printf("Could not open codec\n");
        return 8;
    }

    // Media Playback Pipeline:
    // Input file -> DEMUXER -> AVPackets(Compressed Data) -> DECODER -> AVFrame(Raw Data)

    AVFrame* av_frame = av_frame_alloc();

    if(!av_frame)
    {
        printf("Could not allocate AVFrame\n");
        return 9;
    }

    AVPacket* av_packet = av_packet_alloc();

    if(!av_packet)
    {
        printf("Could not allocate AVPacket\n");
        return 10;
    }

    while(av_read_frame(av_format_context , av_packet) >= 0)
    {
        if(av_packet -> stream_index == video_stream)
        {
            // Decode the Video Packet(AVPacket: Compressed Video Data)
            // to get Video Frames (AVFrame: Raw Video Data)

            // Note: In case of Video, the Packet contains only one Frame.

            // Sending the Video Packet to the decoder to get Video Frames
            returnValue = avcodec_send_packet(video_codec_context , av_packet);
            if(returnValue < 0)
            {
                printf("Error sending packet for decoding\n");
                break;
            }
            returnValue = avcodec_receive_frame(video_codec_context , av_frame);
            if(returnValue == AVERROR(EAGAIN) || returnValue == AVERROR_EOF)
            {
                printf("Some error occurred\n");
            }
            else if(returnValue < 0)
            {
                printf("Failed to decode packet\n");
            }
            else
            {
                // Printing Basic Frame Properties
                printf("Frame Number: %d\n" , video_codec_context -> frame_number);
                printf("Type: %c\n" , av_get_picture_type_char(av_frame -> pict_type));
                printf("Size: %d bytes\n" , av_frame -> pkt_size);
                printf("pts: %ld\n" , av_frame -> pts);
                printf("key_frame: %d\n\n" , av_frame -> key_frame);
            }
        }
        av_packet_unref(av_packet);
    }

    av_frame_free(&av_frame);
    av_packet_free(&av_packet);
    avformat_free_context(av_format_context);
    avcodec_free_context(&video_codec_context);
    avformat_close_input(&av_format_context);

    return 0;
}