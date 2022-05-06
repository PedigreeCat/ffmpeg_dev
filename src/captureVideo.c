#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libavcodec/avcodec.h"
#include "libavdevice/avdevice.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"

#define INPUT_DEVICE "/dev/video0"

void v4l2ListFormats()
{
    AVInputFormat *inFmt = av_find_input_format("v4l2");
    if (inFmt == NULL)
    {
        av_log(NULL, AV_LOG_ERROR, "find v4l2 failed!\n");
        return;
    }

    AVDictionary *options = NULL;
    av_dict_set(&options, "list_formats", "1", 0);
    AVFormatContext *inFmtCtx = avformat_alloc_context();
    // 后三个参数对应ffmpeg的选项: -i, -f, options
    int ret = avformat_open_input(&inFmtCtx, INPUT_DEVICE, inFmt, &options);
    if (ret != 0)
    {
        av_log(NULL, AV_LOG_ERROR, "open input format failed: %s\n", av_err2str(ret));
        return;
    }
    if (inFmtCtx)
    {
        avformat_close_input(&inFmtCtx);
    }
}

void decodeVideo(struct SwsContext *swsCtx, AVCodecContext *decoderCtx, AVFrame *destFrame,
    AVPacket *packet, FILE *dest_fp)
{
    if (avcodec_send_packet(decoderCtx, packet) == 0)
    {
        AVFrame *frame = av_frame_alloc();
        while (avcodec_receive_frame(decoderCtx, frame) >= 0)
        {
            sws_scale(swsCtx, (const uint8_t *const *)frame->data, frame->linesize, 0, decoderCtx->height,
                destFrame->data, destFrame->linesize);
            // yuyv422 yuv422 packed
            // fwrite(frame->data[0], 1, decoderCtx->width * decoderCtx->height * 2, dest_fp);

            // yuv420p
            fwrite(destFrame->data[0], 1, decoderCtx->width * decoderCtx->height, dest_fp);
            fwrite(destFrame->data[1], 1, decoderCtx->width * decoderCtx->height / 4, dest_fp);
            fwrite(destFrame->data[2], 1, decoderCtx->width * decoderCtx->height / 4, dest_fp);
        }
        av_frame_free(&frame);
    }
}

int main(int argc, char **argv)
{
    av_log_set_level(AV_LOG_INFO);
    if (argc < 2)
    {
        av_log(NULL, AV_LOG_ERROR, "Usage: %s <outFileName>\n", argv[0]);
        return -1;
    }
    const char *outFileName = argv[1];

    avdevice_register_all();
    v4l2ListFormats();

    AVFormatContext *inFmtCtx = avformat_alloc_context();
    AVInputFormat *inFmt = av_find_input_format("v4l2");
    if (inFmt == NULL)
    {
        av_log(NULL, AV_LOG_ERROR, "find v4l2 failed!\n");
        goto end;
    }
    AVDictionary *options = NULL;
    av_dict_set(&options, "framerate", "30", 0);
    av_dict_set(&options, "video_size", "640x480", 0);
    int ret = avformat_open_input(&inFmtCtx, INPUT_DEVICE, inFmt, &options);
    if (ret != 0)
    {
        av_log(NULL, AV_LOG_ERROR, "open input failed: %s\n", av_err2str(ret));
        goto end;
    }

    ret = avformat_find_stream_info(inFmtCtx, NULL);
    if (ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "find stram info failed!\n");
        goto end;
    }

    av_find_best_stream(inFmtCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "find video stream failed!\n");
        goto end;
    }
    int videoIndex = ret;
    AVCodecContext *decoderCtx = avcodec_alloc_context3(NULL);

    ret = avcodec_parameters_to_context(decoderCtx, inFmtCtx->streams[videoIndex]->codecpar);
    if (ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "parameters to context failed!");
        goto end;
    }

    AVCodec *decoder = avcodec_find_decoder(decoderCtx->codec_id);
    if (decoder == NULL)
    {
        av_log(NULL, AV_LOG_ERROR, "find decoder failed, id: %d\n", (int)decoderCtx->codec_id);
        goto end;
    }

    ret = avcodec_open2(decoderCtx, decoder, NULL);
    if (ret != 0)
    {
        av_log(NULL, AV_LOG_ERROR, "open decoder failed!\n");
        goto end;
    }

    AVFrame *destFrame = av_frame_alloc();
    enum AVPixelFormat destPixFmt = AV_PIX_FMT_YUV420P;
    int bufferSize = av_image_get_buffer_size(destPixFmt, decoderCtx->width, decoderCtx->height, 1);
    uint8_t *outBuffer = av_malloc(bufferSize);
    av_image_fill_arrays(destFrame->data, destFrame->linesize, outBuffer, destPixFmt, \
        decoderCtx->width, decoderCtx->height, 1);
    
    struct SwsContext *swsCtx = sws_getContext(decoderCtx->width, decoderCtx->height, \
        decoderCtx->pix_fmt, decoderCtx->width, decoderCtx->height, destPixFmt, 0, NULL, NULL, NULL);
    if (swsCtx == NULL)
    {
        ret = -1;
        goto end;
    }

    FILE *dest_fp = fopen(outFileName, "wb+");
    if (dest_fp == NULL)
    {
        av_log(NULL, AV_LOG_ERROR, "open output file %s failed!\n", outFileName);
        ret = -1;
        goto end;
    }

    AVPacket packet;
    av_init_packet(&packet);

    while (1)
    {
        if (av_read_frame(inFmtCtx, &packet) == 0)
        {
            decodeVideo(swsCtx, decoderCtx, destFrame, &packet, dest_fp);
        }
        av_packet_unref(&packet);
    }
    decodeVideo(swsCtx, decoderCtx, destFrame, NULL, dest_fp);
    
end:
    if (inFmtCtx)
    {
        avformat_free_context(inFmtCtx);
    }
    if (decoderCtx)
    {
        avcodec_free_context(&decoderCtx);
    }
    if (dest_fp)
    {
        fclose(dest_fp);
    }
    if (outBuffer)
    {
        av_freep(&outBuffer);
    }
    if (destFrame)
    {
        av_frame_free(&destFrame);
    }
    return ret;
}