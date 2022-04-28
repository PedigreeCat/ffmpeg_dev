#include "libavutil/avutil.h"
#include "libavformat/avformat.h"

int frameCount = 0;

int decodeVideo(AVCodecContext *codecCtx, AVPacket *packet, FILE *dest_fp)
{
    int ret = avcodec_send_packet(codecCtx, packet);
    if (ret != 0)
    {
        av_log(NULL, AV_LOG_ERROR, "send packet failed: %s\n", av_err2str(ret));
        av_packet_unref(packet);
        return -1;
    }
    AVFrame *frame = av_frame_alloc();
    while (avcodec_receive_frame(codecCtx, frame) == 0)
    {
        fwrite(frame->data[0], 1, codecCtx->width * codecCtx->height, dest_fp);
        fwrite(frame->data[1], 1, codecCtx->width * codecCtx->height / 4, dest_fp);
        fwrite(frame->data[2], 1, codecCtx->width * codecCtx->height / 4, dest_fp);
        frameCount++;
        // av_log(NULL, AV_LOG_INFO, "frameCount: %d\n", frameCount);
        av_log(NULL, AV_LOG_INFO,
            "linesize[0] = %d, linesize[1] = %d, linesize[2] = %d, width = %d, height = %d\n",
            frame->linesize[0], frame->linesize[1], frame->linesize[2], codecCtx->width, codecCtx->height);
    }
    if (frame)
    {
        av_frame_free(&frame);
    }
}

int main(int argc, char **argv)
{
    av_log_set_level(AV_LOG_INFO);
    if (argc < 3)
    {
        av_log(NULL, AV_LOG_ERROR, "Usage: %s <infileName> <outfileName> \n", argv[0]);
        return -1;
    }

    const char *inFileName = argv[1];
    const char *outFileName = argv[2];

    AVFormatContext *inFmtCtx = NULL;

    int ret = avformat_open_input(&inFmtCtx, inFileName, NULL, NULL);
    if (ret != 0)
    {
        av_log(NULL, AV_LOG_ERROR, "open input file: %s failed: %s\n", inFileName, av_err2str(ret));
        return -1;
    }

    ret = avformat_find_stream_info(inFmtCtx, NULL);
    if (ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "find input stream info failed: %s\n", av_err2str(ret));
        goto fail;
    }

    ret = av_find_best_stream(inFmtCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "alloc avcodec context failed!\n");
    }
    int videoIndex = ret;

    AVCodecContext *codecCtx = avcodec_alloc_context3(NULL);
    if (codecCtx == NULL)
    {
        av_log(NULL, AV_LOG_ERROR, "alloc avcodec context failed!\n");
        ret = -1;
        goto fail;
    }

    avcodec_parameters_to_context(codecCtx, inFmtCtx->streams[videoIndex]->codecpar);

    AVCodec *decoder = avcodec_find_decoder(codecCtx->codec_id);
    if (decoder == NULL)
    {
        av_log(NULL, AV_LOG_ERROR, "find decoder failed, code_id: %d\n", codecCtx->codec_id);
        ret = -1;
        goto fail;
    }

    ret = avcodec_open2(codecCtx, decoder, NULL);
    if (ret != 0)
    {
        av_log(NULL, AV_LOG_ERROR, "open decoder failed: %s\n", av_err2str(ret));
        goto fail;
    }

    FILE *dest_fp = fopen(outFileName, "wb+");
    if (dest_fp == NULL) 
    {
        av_log(NULL, AV_LOG_ERROR, "open outfile: %s failed!\n", outFileName);
        ret = -1;
        goto fail;
    }

    AVPacket packet;
    av_init_packet(&packet);

    while (av_read_frame(inFmtCtx, &packet) >= 0)
    {
        if (packet.stream_index == videoIndex)
        {
            if (decodeVideo(codecCtx, &packet, dest_fp) == -1)
            {
                ret = -1;
                av_packet_unref(&packet);
                goto fail;
            }
        }
        av_packet_unref(&packet);
    }
    // flush decoder
    decodeVideo(codecCtx, NULL, dest_fp);

fail:
    if (inFmtCtx)
    {
        avformat_close_input(&inFmtCtx);
    }
    if (codecCtx)
    {
        avcodec_free_context(&codecCtx);
    }
    if (dest_fp)
    {
        fclose(dest_fp);
    }
    return ret;    
}