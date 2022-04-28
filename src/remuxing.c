#include "libavutil/log.h"
#include "libavformat/avformat.h"

int main(int argc, char **argv)
{
	av_log_set_level(AV_LOG_DEBUG);
	if (argc < 3)
	{
		av_log(NULL, AV_LOG_ERROR, "Usage: %s <infileName> <oufileName>\n", argv[0]);
		return -1;
	}
	const char *inFileName = argv[1];
	const char *outFileName = argv[2];

	AVFormatContext *inFmtCtx = NULL;
	int ret = avformat_open_input(&inFmtCtx, inFileName, NULL, NULL);
	if (ret != 0)
	{
		av_log(NULL, AV_LOG_ERROR, "open input format failed: %s\n", av_err2str(ret));
		return -1;
	}

	ret = avformat_find_stream_info(inFmtCtx, NULL);
	if (ret != 0)
	{
		av_log(NULL, AV_LOG_ERROR, "find input stream failed: %s\n", av_err2str(ret));
		goto fail;
	}

	AVFormatContext *outFmtCtx = NULL;
	ret = avformat_alloc_output_context2(&outFmtCtx, NULL, NULL, outFileName);
	if (ret < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "alloc output format failed: %s\n", av_err2str(ret));
		goto fail;
	}

	int streamCount = inFmtCtx->nb_streams;
	int *handleStreamIndexArray = av_malloc_array(streamCount, sizeof(int));
	if (handleStreamIndexArray == NULL)
	{
		av_log(NULL, AV_LOG_ERROR, "malloc handle stream index array failed\n");
		goto fail;
	}

	int streamIndex = 0;
	for (int i = 0; i < streamCount; i++)
	{
		AVStream *inStream = inFmtCtx->streams[i];
		if (inStream->codecpar->codec_type != AVMEDIA_TYPE_VIDEO &&
			inStream->codecpar->codec_type != AVMEDIA_TYPE_AUDIO &&
			inStream->codecpar->codec_type != AVMEDIA_TYPE_SUBTITLE)
		{
			handleStreamIndexArray[i] = -1;
			continue;
		}
		handleStreamIndexArray[i] = streamIndex++;
		AVStream *outStream = NULL;
		outStream = avformat_new_stream(outFmtCtx, NULL);
		if (outStream == NULL)
		{
			av_log(NULL, AV_LOG_ERROR, "new output stream failed\n");
			goto fail;
		}
		avcodec_parameters_copy(outStream->codecpar, inStream->codecpar);
		outStream->codecpar->codec_tag = 0;
	}

	/* 安全起见加层判断 */
	if (!(outFmtCtx->oformat->flags & AVFMT_NOFILE))
	{
		ret = avio_open(&outFmtCtx->pb, outFileName, AVIO_FLAG_WRITE);
		if (ret < 0)
		{
			av_log(NULL, AV_LOG_ERROR, "open %s file failed\n", outFileName);
			goto fail;
		}
	}

	ret = avformat_write_header(outFmtCtx, NULL);
	if (ret < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "write header failed: %s\n", av_err2str(ret));
		goto fail;
	}

	AVPacket packet;
	av_init_packet(&packet);

	while (av_read_frame(inFmtCtx, &packet) == 0)
	{
		if (packet.stream_index >= streamCount ||
			handleStreamIndexArray[packet.stream_index] == -1)
		{
			av_packet_unref(&packet);
			continue;
		}

		AVStream *inStream = inFmtCtx->streams[packet.stream_index];
		AVStream *outStream = outFmtCtx->streams[packet.stream_index];

		packet.stream_index = handleStreamIndexArray[packet.stream_index];
		packet.pts = av_rescale_q(packet.pts, inStream->time_base, outStream->time_base);
		packet.dts = av_rescale_q(packet.dts, inStream->time_base, outStream->time_base);
		packet.duration = av_rescale_q(packet.duration, inStream->time_base, outStream->time_base);
		packet.pos = -1;

		ret = av_interleaved_write_frame(outFmtCtx, &packet);
		// check write success
		av_packet_unref(&packet);
	}

	av_write_trailer(outFmtCtx);
	// check write success


fail:
	if (inFmtCtx) { avformat_close_input(&inFmtCtx); }
	if (outFmtCtx && !(outFmtCtx->oformat->flags & AVFMT_NOFILE)) { avio_closep(&outFmtCtx->pb); }
	if (outFmtCtx) { avformat_free_context(outFmtCtx); }
	if (handleStreamIndexArray) { av_freep(&handleStreamIndexArray); }
	return ret;
}