#include "libavutil/avutil.h"
#include "libavcodec/avcodec.h"

int encodeAudio(AVCodecContext *encoderCtx, AVFrame *frame, AVPacket *packet, FILE *dest_fp)
{
    int ret = avcodec_send_frame(encoderCtx, frame);
    if (ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "send frame to encoder failed: %s\n", av_err2str(ret));
        return -1;
    }
    while (ret >= 0)
    {
        ret = avcodec_receive_packet(encoderCtx, packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
        {
            return 0;
        }
        else if (ret < 0)
        {
            av_log(NULL, AV_LOG_ERROR, "encode frame failed: %s\n", av_err2str(ret));
            return -1;
        }
        fwrite(packet->data, 1, packet->size, dest_fp);
    }
}

int main(int argc, char **argv)
{
    av_log_set_level(AV_LOG_INFO);
    if (argc < 5)
    {
        av_log(NULL, AV_LOG_ERROR, "Usage: %s <sampleRate> <channels> <infile> <outfile>\n", argv[0]);
        return -1;
    }
    const char *inFileName = argv[3];
    const char *outFileName = argv[4];

    AVFrame *frame = av_frame_alloc();
    frame->sample_rate = atoi(argv[1]);
    frame->channels = atoi(argv[2]);
    frame->channel_layout = AV_CH_LAYOUT_STEREO;
    // libfdk_aac default encoder FMT is AV_SAMPLE_FMT_S16
    frame->format = AV_SAMPLE_FMT_S16;
    frame->nb_samples = 1024;
    av_frame_get_buffer(frame, 0);

    int ret = 0;
    AVCodec *encoder = avcodec_find_encoder_by_name("libfdk_aac");
    if (encoder == NULL)
    {
        av_log(NULL, AV_LOG_ERROR, "find encoder failed!\n");
        ret = -1;
        goto end;
    }

    AVCodecContext *encoderCtx = avcodec_alloc_context3(encoder);
    if (encoderCtx== NULL)
    {
        av_log(NULL, AV_LOG_ERROR, "alloc encoder context failed!\n");
        ret = -1;
        goto end;
    }

    encoderCtx->sample_fmt = frame->format;
    encoderCtx->sample_rate = frame->sample_rate;
    encoderCtx->channels = frame->channels;
    encoderCtx->channel_layout = frame->channel_layout;

    ret = avcodec_open2(encoderCtx, encoder, NULL);
    if (ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "open encoder failed: %s\n", av_err2str(ret));
        goto end;
    }

    FILE *src_fp = fopen(inFileName, "rb");
    if (src_fp == NULL)
    {
        av_log(NULL, AV_LOG_ERROR, "open infile: %s failed!\n", inFileName);
        ret = -1;
        goto end;
    }

    FILE *dest_fp = fopen(outFileName, "wb+");
    if (dest_fp == NULL)
    {
        av_log(NULL, AV_LOG_ERROR, "open outfile: %s failed!\n", outFileName);
        ret = -1;
        goto end;
    }

    AVPacket packet;
    av_init_packet(&packet);
    while (1)
    {
        // packed L R L R L R
        // 2 * 2 * 1024 = 4096
        int readSize = fread(frame->data[0], 1, frame->linesize[0], src_fp);
        if (readSize == 0)
        {
            av_log(NULL, AV_LOG_INFO, "finish read infile!\n");
            break;
        }
        encodeAudio(encoderCtx, frame, &packet, dest_fp);
    }
    encodeAudio(encoderCtx, NULL, &packet, dest_fp);
end:
    if (frame)
    {
        av_frame_free(&frame);
    }
    if (encoderCtx)
    {
        avcodec_free_context(&encoderCtx);
    }
    if (src_fp)
    {
        fclose(src_fp);
    }
    if (dest_fp)
    {
        fclose(dest_fp);
    }
    return 0;
}