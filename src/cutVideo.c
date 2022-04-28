#include "libavutil/log.h"
#include "libavformat/avformat.h"

int main(int argc, char **argv)
{
    av_log_set_level(AV_LOG_INFO);
    // programeName srcfile 5 20 outfile
    if (argc < 5)
    {
        av_log(NULL, AV_LOG_ERROR, "Usage: %s <infileName> <startTime> <endTime> <outName>\n", argv[0]);
        return -1;
    }
    const char *inFileName = argv[1];
    int startTime = atoi(argv[2]);
    int endTime = atoi(argv[3]);
    const char *outFileName = argv[4];

    AVFormatContext *inFmtCtx = NULL;
    int ret = avformat_open_input(&inFmtCtx, inFileName, NULL, NULL);
    if (ret != 0)
    {
        return -1;
    }

    ret = avformat_find_stream_info(inFmtCtx, NULL);
    if (ret < 0) 
    {
        goto fail;
    }

    AVFormatContext *outFmtCtx = NULL;
    ret = avformat_alloc_output_context2(&outFmtCtx, NULL, NULL, outFileName);
    if (ret < 0)
    {
        goto fail;
    }

    for (int i = 0; i < inFmtCtx->nb_streams; i++)
    {
        AVStream *inStream = inFmtCtx->streams[i];
        AVStream *outStream = avformat_new_stream(outFmtCtx, NULL);
        if (outStream == NULL)
        {
            ret = -1;
            goto fail;
        }

        ret = avcodec_parameters_copy(outStream->codecpar, inStream->codecpar);
        if (ret < 0)
        {
            goto fail;
        }
        outStream->codecpar->codec_tag = 0;
    }

    if (!(outFmtCtx->oformat->flags & AVFMT_NOFILE))
    {
        ret = avio_open(&outFmtCtx->pb, outFileName, AVIO_FLAG_WRITE);
        if (ret < 0)
        {
            goto fail;
        }
    }
    ret = avformat_write_header(outFmtCtx, NULL);
    if (ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "write header failed: %s\n", av_err2str(ret));
        goto fail;
    }

    av_seek_frame(inFmtCtx, -1, startTime * AV_TIME_BASE, AVSEEK_FLAG_ANY);
    if (ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "seek frame failed: %s\n", av_err2str(ret));
        goto fail;
    }

    int64_t *startPTS = av_malloc_array(inFmtCtx->nb_streams, sizeof(int64_t));
    memset(startPTS, 0, inFmtCtx->nb_streams * sizeof(int64_t));
    int64_t *startDTS = av_malloc_array(inFmtCtx->nb_streams, sizeof(int64_t));
    memset(startDTS, 0, inFmtCtx->nb_streams * sizeof(int64_t));

    AVPacket packet;
    av_init_packet(&packet);
    while (av_read_frame(inFmtCtx, &packet) == 0)
    {
        AVStream *inStream = inFmtCtx->streams[packet.stream_index];
        AVStream *outStream = outFmtCtx->streams[packet.stream_index];
        if (endTime < packet.pts * av_q2d(inStream->time_base))
        {
            av_packet_unref(&packet);
            break;
        }

        if (startPTS[packet.stream_index] == 0)
        {
            startPTS[packet.stream_index] = packet.pts;
        }
        if (startDTS[packet.stream_index] == 0)
        {
            startDTS[packet.stream_index] = packet.dts;
        }
        packet.pts = av_rescale_q(packet.pts - startPTS[packet.stream_index],
            inStream->time_base, outStream->time_base);
        packet.dts = av_rescale_q(packet.dts - startDTS[packet.stream_index],
            inStream->time_base, outStream->time_base);
        if (packet.pts < 0)
        {
            packet.pts = 0;
        }
        if (packet.dts < 0)
        {
            packet.dts = 0;
        }
        packet.duration = av_rescale_q(packet.duration, inStream->time_base, outStream->time_base);
        packet.pos = -1;
        if (packet.pts < packet.dts)
        {
            av_packet_unref(&packet);
            continue;
        }
        ret = av_interleaved_write_frame(outFmtCtx, &packet);
        if (ret < 0)
        {
            /* av_interleaved_write_frame接口会对写入的pts和dts进行校验码 */
            av_log(NULL, AV_LOG_ERROR, "write frame failed: %s\n", av_err2str(ret));
            av_packet_unref(&packet);
            break;
        }
        av_packet_unref(&packet);
    }

    av_write_trailer(outFmtCtx);
fail:
    if (inFmtCtx)
    {
        avformat_close_input(&inFmtCtx);
    }
    if (outFmtCtx && !(outFmtCtx->oformat->flags & AVFMT_NOFILE))
    {
        avio_closep(&outFmtCtx->pb);
    }
    if (outFmtCtx)
    {
        avformat_free_context(outFmtCtx);
    }
    if (startPTS)
    {
        av_freep(&startPTS);
    }
    if (startDTS)
    {
        av_freep(&startDTS);
    }
    return ret;
}