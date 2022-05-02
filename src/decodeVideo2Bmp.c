#include "libavutil/avutil.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/parseutils.h"
#include "libavutil/imgutils.h"

#define WORD uint16_t
#define DWORD uint32_t
#define LONG int32_t

#pragma pack(2)
/* 文件头结构体 */
typedef struct tagBITMAPFILEHEADER {
	WORD	bfType;			// 固定为0x4d42，即'B','M'
	DWORD	bfSize;			// 整个bmp文件大小
	WORD	bfReserved1;	// 保留字
    WORD	bfReserved2;	// 保留字	
	DWORD 	bfOffBits;		// 实际位图数据的偏移字节数，即前三个部分长度之和
} BITMAPFILEHEADER;

/* 信息头结构体 */
typedef struct tagBITMAPINFOHEADER {
    DWORD	biSize;			// 表示struct tagBITMAPINFOHEADER结构体的长度，设为40
    LONG	biWidth;		// bmp图片宽度
    LONG	biHeight;		// bmp图片高度
    WORD	biPlanes;		// bmp图片平面数，乘以负1
    WORD	biBitCount;		// bmp图片的颜色位数，即1位图（单色或二值图像）、8位图、16位图、24位图、32位图等
    DWORD	biCompression;	// 图片的压缩属性，bmp图片是不压缩的，设为0，表示不压缩
    DWORD	biSizeImage;	// 实际位图数据占用的字节数，若biCompression为0，可省略
    LONG	biXPelsPerMeter;// X方向分辨率，可省略
    LONG	biYPelsPerMeter;// Y方向分辨率，可省略
    DWORD	biClrUsed;		// 使用了多少个颜色索引标，如果为0，则表示默认值（2^biBitCount）
    DWORD	biClrImportant;	// 重要颜色数，如果为0，则表示所有颜色都是重要的
} BITMAPINFOHEADER;

void saveBMP(const char *fileName, unsigned char *rgbData, int width, int height)
{
    int bmpDataSize = width * height * 3;
    BITMAPFILEHEADER bmpFileHeader = {0};
    bmpFileHeader.bfType = 0x4d42;
    bmpFileHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + bmpDataSize;
    bmpFileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    BITMAPINFOHEADER bmpInfoHeader = {0};
    bmpInfoHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmpInfoHeader.biWidth = width;
    bmpInfoHeader.biHeight = height * (-1);
    bmpInfoHeader.biPlanes = 1;
    bmpInfoHeader.biBitCount = 24;
    bmpInfoHeader.biCompression = 0;
    bmpInfoHeader.biSizeImage = 0;
    bmpInfoHeader.biXPelsPerMeter = 0;
    bmpInfoHeader.biYPelsPerMeter = 0;
    bmpInfoHeader.biClrUsed = 0;
    bmpInfoHeader.biClrImportant = 0;
    
    // 确保当前路径的文件夹存在
    FILE *fp = fopen(fileName, "wb");
    fwrite(&bmpFileHeader, 1, sizeof(BITMAPFILEHEADER), fp);
    fwrite(&bmpInfoHeader, 1, sizeof(BITMAPINFOHEADER), fp);
    fwrite(rgbData, 1, bmpDataSize, fp);
    fclose(fp);
    return;
}

int frameCount = 0;

int decodeVideo(AVCodecContext *codecCtx, AVPacket *packet, struct SwsContext *swsCtx,
                int destWidth, int destHeight, AVFrame *destFrame, FILE *dest_fp)
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
        sws_scale(swsCtx, frame->data, frame->linesize, 0, codecCtx->height,
                    destFrame->data, destFrame->linesize);
#if 0
        // write data as YUV420P
        fwrite(destFrame->data[0], 1, destWidth * destHeight, dest_fp);
        fwrite(destFrame->data[1], 1, destWidth * destHeight / 4, dest_fp);
        fwrite(destFrame->data[2], 1, destWidth * destHeight / 4, dest_fp);
#else
        // write data as RGB
        fwrite(destFrame->data[0], 1, destWidth * destHeight * 3, dest_fp);
        char bmpFileName[200] = {0};
        snprintf(bmpFileName, sizeof(bmpFileName), "./picture/%d.bmp", frameCount);
        saveBMP(bmpFileName, destFrame->data[0], destWidth, destHeight);
#endif
        frameCount++;
        av_log(NULL, AV_LOG_INFO,\
            "frameCount: %d, linesize[0] = %d, linesize[1] = %d, linesize[2] = %d, width = %d, height = %d\r",\
            frameCount, destFrame->linesize[0], destFrame->linesize[1], destFrame->linesize[2], destWidth, destHeight);
    }
    if (frame)
    {
        av_frame_free(&frame);
    }
}

int main(int argc, char **argv)
{
    av_log_set_level(AV_LOG_INFO);
    if (argc < 4)
    {
        av_log(NULL, AV_LOG_ERROR, "Usage: %s <infileName> <outfileName> <width*height>\n", argv[0]);
        return -1;
    }

    const char *inFileName = argv[1];
    const char *outFileName = argv[2];
    const char *destVideoSizeString = argv[3];
    int destWidth = 0, destHeight = 0;
    int ret = av_parse_video_size(&destWidth, &destHeight, destVideoSizeString);
    if (ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "invalid video size: %s\n", destVideoSizeString);
        return -1;
    }
    av_log(NULL, AV_LOG_INFO, "destWidth: %d, destHeight: %d\n", destWidth, destHeight);

    AVFormatContext *inFmtCtx = NULL;

    ret = avformat_open_input(&inFmtCtx, inFileName, NULL, NULL);
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

    // enum AVPixelFormat destPixFmt = codecCtx->pix_fmt;
    // enum AVPixelFormat destPixFmt = AV_PIX_FMT_RGB24; // write data as RGB24
    enum AVPixelFormat destPixFmt = AV_PIX_FMT_BGR24;
    struct SwsContext *swsCtx = 
        sws_getContext(codecCtx->width, codecCtx->height, codecCtx->pix_fmt,
                        destWidth, destHeight, destPixFmt, SWS_FAST_BILINEAR, NULL, NULL, NULL);
    if (swsCtx == NULL)
    {
        av_log(NULL, AV_LOG_ERROR, "get sws context failed!\n");
        ret = -1;
        goto fail;
    }

    AVFrame *destFrame = av_frame_alloc();
    uint8_t *outBuffer = av_malloc(av_image_get_buffer_size(destPixFmt, destWidth, destHeight, 1));
    av_image_fill_arrays(destFrame->data, destFrame->linesize, outBuffer, destPixFmt, destWidth, destHeight, 1);

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
            if (decodeVideo(codecCtx, &packet, swsCtx, destWidth, destHeight, destFrame, dest_fp) == -1)
            {
                ret = -1;
                av_packet_unref(&packet);
                goto fail;
            }
        }
        av_packet_unref(&packet);
    }
    // flush decoder
    decodeVideo(codecCtx, &packet, swsCtx, destWidth, destHeight, destFrame, dest_fp);

fail:
    av_log(NULL, AV_LOG_INFO, "\n");
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
    if (destFrame)
    {
        av_frame_free(&destFrame);
    }
    if (outBuffer)
    {
        av_freep(&outBuffer);
    }
    return ret;    
}