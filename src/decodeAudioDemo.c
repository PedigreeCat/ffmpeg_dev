#include "libavutil/avutil.h"
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"

int decodeAudio(AVCodecContext *decoderCtx, AVPacket *packet, AVFrame *frame, FILE *dest_fp)
{
    int ret = avcodec_send_packet(decoderCtx, packet);
    if (ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "send packet to decoder failed: %s\n", av_err2str(ret));
        return -1;
    }
    int i = 0, channel = 0;
    while (ret >= 0)
    {
        ret = avcodec_receive_frame(decoderCtx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
        {
            return 0;
        }
        else if (ret < 0)
        {
            av_log(NULL, AV_LOG_ERROR, "decode packet failed: %s\n", av_err2str(ret));
        }

        int dataSize = av_get_bytes_per_sample(decoderCtx->sample_fmt);
        if (dataSize < 0)
        {
            av_log(NULL, AV_LOG_ERROR, "get bytes failed!\n");
            return -1;
        }
        // frame    fltp    2   LC
        /**
         * data[0] L L L L
         * data[1] R R R R
         * 
         * ==> L R L R L R L R
         */
        for (i = 0; i < frame->nb_samples; i++)
        {
            for (channel = 0; channel < decoderCtx->channels; channel++)
            {
                fwrite(frame->data[channel] + dataSize * i, 1, dataSize, dest_fp);
            }
        }
    }
    return 0;
}

int main(int argc, char **argv)
{
    av_log_set_level(AV_LOG_INFO);
    if (argc < 3)
    {
        av_log(NULL, AV_LOG_ERROR, "Usage: %s <infile> <outfile> \n", argv[0]);
        return -1;
    }
    const char *inFileName = argv[1];
    const char *outFileName = argv[2];
    int ret = 0;
    AVFormatContext *inFmtCtx = NULL;
    ret = avformat_open_input(&inFmtCtx, inFileName, NULL, NULL);
    if (ret != 0)
    {
        av_log(NULL, AV_LOG_ERROR, "open input file failed: %s\n", av_err2str(ret));
        return -1;
    }

    ret = avformat_find_stream_info(inFmtCtx, NULL);
    if (ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "find stream info failed: %s\n", av_err2str(ret));
        goto end;
    }

    ret = av_find_best_stream(inFmtCtx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if (ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "find best stream failed: %s\n", av_err2str(ret));
        goto end;
    }
    
    int audioIndex = ret;
    
    AVCodecContext *decoderCtx = avcodec_alloc_context3(NULL);
    if (decoderCtx == NULL)
    {
        av_log(NULL, AV_LOG_ERROR, "alloc context failed\n");
        ret = -1;
        goto end;
    }

    avcodec_parameters_to_context(decoderCtx, inFmtCtx->streams[audioIndex]->codecpar);

    AVCodec *decoder = avcodec_find_decoder(decoderCtx->codec_id);
    if (decoder == NULL)
    {
        av_log(NULL, AV_LOG_ERROR, "find decoder %d failed!\n", decoderCtx->codec_id);
        ret = -1;
        goto end;
    }
    ret = avcodec_open2(decoderCtx, decoder, NULL);
    if (ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "open decoder failed: %s\n", av_err2str(ret));
        goto end;
    }

    AVFrame *frame = av_frame_alloc();
    int frameSize = av_samples_get_buffer_size(NULL, decoderCtx->channels, frame->nb_samples, decoderCtx->sample_fmt, 1);
    uint8_t *frameBuffer = av_malloc(frameSize);
    avcodec_fill_audio_frame(frame, decoderCtx->channels, decoderCtx->sample_fmt, frameBuffer, frameSize, 1);
    
    FILE *dest_fp = fopen(outFileName, "wb+");
    if (dest_fp == NULL)
    {
        av_log(NULL, AV_LOG_ERROR, "open outfile %s failed!\n", outFileName);
        ret = -1;
        goto end;
    }
    
    AVPacket packet;
    av_init_packet(&packet);

    while (av_read_frame(inFmtCtx, &packet) >= 0)
    {
        if (packet.stream_index == audioIndex)
        {
            ret = decodeAudio(decoderCtx, &packet, frame, dest_fp);
            if (ret != 0)
            {
                av_log(NULL, AV_LOG_ERROR, "decode audio failed!\n");
                goto end;
            }
        }
        av_packet_unref(&packet);
    }
    decodeAudio(decoderCtx, NULL, frame, dest_fp);
end:
    if (inFmtCtx)
    {
        avformat_close_input(&inFmtCtx);
    }
    if (decoderCtx)
    {
        avcodec_free_context(&decoderCtx);
    }
    if (frame)
    {
        av_frame_free(&frame);
    }
    if (frameBuffer)
    {
        av_freep(&frameBuffer);
    }
    if (dest_fp)
    {
        fclose(dest_fp);
    }
    return ret;
}