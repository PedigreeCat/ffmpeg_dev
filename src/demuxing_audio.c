#include "libavutil/avutil.h"
#include "libavformat/avformat.h"

const int sampleFrequencyTable[] = {
    96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350
};

#define ADTS_HEADER_LEN (7)

int getADTSHeader(char *adtsHeader, int packetSize, int profile, int sampleRate, int channels)
{
    int sampleFrequencyIndex = 3; // 默认采样率对应48000Hz
    int adtsLength = packetSize + ADTS_HEADER_LEN;

    for (int i = 0; i < sizeof(sampleFrequencyTable) / sizeof(sampleFrequencyTable[0]); i++)
    {
        if (sampleRate == sampleFrequencyTable[i])
        {
            sampleFrequencyIndex = i;
            break;
        }
    }

    adtsHeader[0] = 0xff;               // syncword:0xfff                                  12 bits
 
    adtsHeader[1] = 0xf0;               // syncword:0xfff
    adtsHeader[1] |= (0 << 3);          // MPEG Version: 0 for MPEG-4, 1 for MPEG-2         1 bit
    adtsHeader[1] |= (0 << 1);          // Layer:00                                         2 bits
    adtsHeader[1] |= 1;                 // protection absent:1                              1 bit
 
    adtsHeader[2] = (profile << 6);     // profile:0 for Main, 1 for LC, 2 for SSR          2 bits
    adtsHeader[2] |= (sampleFrequencyIndex & 0x0f) << 2; // sampling_frequency_index        4 bits
    adtsHeader[2] |= (0 << 1);                           // private bit:0                   1 bit
    adtsHeader[2] |= (channels & 0x04) >> 2;             // channel configuration           3 bits
 
    adtsHeader[3] = (channels & 0x03) << 6;              // channel configuration
    adtsHeader[3] |= (0 << 5);                           // original_copy                   1 bit
    adtsHeader[3] |= (0 << 4);                           // home                            1 bit
    adtsHeader[3] |= (0 << 3);                           // copyright_identification_bit    1 bit
    adtsHeader[3] |= (0 << 2);                           // copytight_identification_start  1 bit
    adtsHeader[3] |= ((adtsLength & 1800) >> 11);        // AAC frame length               13 bits

    adtsHeader[4] = (uint8_t)((adtsLength & 0x7f8) >> 3);// AAC frame length
    adtsHeader[5] = (uint8_t)((adtsLength & 0x7) << 5);  // AAC frame length
    adtsHeader[5] |= 0x1f;                               // buffer fullness:0x7ff          11 bits
    adtsHeader[6] = 0xfc;                                // buffer fullness:0x7ff
    return 0;
}

int main(int argc, char **argv)
{
    av_log_set_level(AV_LOG_DEBUG);
    if (argc < 3)
    {
        av_log(NULL, AV_LOG_ERROR, "Usage: %s infile outfile\n", argv[0]);
        return -1;
    }
    const char *infileName = argv[1];
    const char *outfileName = argv[2];

    AVFormatContext *inFormatCtx = NULL;
    int ret = avformat_open_input(&inFormatCtx, infileName, NULL, NULL);
    if (ret != 0)
    {
        av_log(NULL, AV_LOG_ERROR, "open input file format failed");
    }

    ret = avformat_find_stream_info(inFormatCtx, NULL);
    if (ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "find stream info failed: %s\n", av_err2str(ret));
        avformat_close_input(&inFormatCtx);
        return -1;
    }

    int audioIndex = av_find_best_stream(inFormatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if (audioIndex < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "find best stream failed, index is %d\n", audioIndex);
    }
    av_log(NULL, AV_LOG_INFO, "the audio index is %d\n", audioIndex);

    AVPacket packet;
    av_init_packet(&packet);

    FILE *dest_fp = fopen(outfileName, "wb");
    if (dest_fp == NULL)
    {
        av_log(NULL, AV_LOG_ERROR, "open %s file failed\n", outfileName);
        avformat_close_input(&inFormatCtx);
        return -1;
    }

    while (av_read_frame(inFormatCtx, &packet) == 0)
    {
        if (packet.stream_index == audioIndex)
        {
            // av_log(NULL, AV_LOG_INFO, "p%d s%d c%d ",
            //     inFormatCtx->streams[audioIndex]->codecpar->profile,
            //     inFormatCtx->streams[audioIndex]->codecpar->sample_rate,
            //     inFormatCtx->streams[audioIndex]->codecpar->channels);
            char adtsHeader[ADTS_HEADER_LEN] = {0};
            getADTSHeader(adtsHeader, packet.size,
                inFormatCtx->streams[audioIndex]->codecpar->profile,
                inFormatCtx->streams[audioIndex]->codecpar->sample_rate,
                inFormatCtx->streams[audioIndex]->codecpar->channels);

            if (sizeof(adtsHeader) != fwrite(adtsHeader, 1, sizeof(adtsHeader), dest_fp) ||
                packet.size != fwrite(packet.data, 1, packet.size, dest_fp))
            {
                av_log(NULL, AV_LOG_ERROR, "write file failed!\n");
                fclose(dest_fp);
                avformat_close_input(&inFormatCtx);
                return -1;
            }
        }
        av_packet_unref(&packet);
    }

    if (inFormatCtx)
    {
        avformat_close_input(&inFormatCtx);
    }
    if (dest_fp)
    {
        fclose(dest_fp);
    }

    return 0;
}