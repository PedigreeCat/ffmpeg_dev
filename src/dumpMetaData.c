#include "libavutil/log.h"
#include "libavformat/avformat.h"

int main(int argc, char **argv)
{
	av_log_set_level(AV_LOG_DEBUG);

	if (argc < 2)
	{
		av_log(NULL, AV_LOG_ERROR, "Usage: %s infileName.\n", argv[0]);
		return -1;
	}
	const char *infileName = argv[1];
	AVFormatContext *pFormatCtx = NULL;

	int ret = avformat_open_input(&pFormatCtx, infileName, NULL, NULL);
	if (ret != 0)
	{
		av_log(NULL, AV_LOG_ERROR, "open input file:%s failed: %s\n", infileName, av_err2str(ret));
		return -1;
	}
	av_dump_format(pFormatCtx, 0, infileName, 0);
	avformat_close_input(&pFormatCtx);

	return 0;
}