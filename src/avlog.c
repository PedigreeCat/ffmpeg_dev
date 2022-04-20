#include "libavutil/log.h"
#include "libavutil/ffversion.h"

int main(int argc, char **argv)
{
    av_log_set_level(AV_LOG_INFO);
    av_log(NULL, AV_LOG_ERROR, "this is error log level!\n");
    av_log(NULL, AV_LOG_INFO, "this is info log level, %d\n", argc);
    av_log(NULL, AV_LOG_WARNING, "this is warn log level, %s\n", argv[0]);
    av_log(NULL, AV_LOG_DEBUG, "this is debug log level!\n");
    av_log(NULL, AV_LOG_INFO, "ffmpeg version %s\n", FFMPEG_VERSION);
    return 0;
}