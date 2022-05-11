#include "SDL2/SDL.h"

Uint8 *g_audioData;
Uint32 g_audioDataSize;

void fillAudio(void *userdata, Uint8 *stream, int len)
{
    if (g_audioDataSize == 0)
    {
        return;
    }
    memset(stream, 0, len);
    SDL_Log("len : %d", len);
    len = (len > g_audioDataSize ? g_audioDataSize : len);
    SDL_MixAudio(stream, g_audioData, len, SDL_MIX_MAXVOLUME);
    g_audioData += len;
    g_audioDataSize -= len;
}

int main(int argc, char **argv)
{
    SDL_Init(SDL_INIT_VIDEO);

    const char *fileName = argv[1];

    SDL_AudioSpec desired;
    desired.channels = 1;
    desired.format = AUDIO_F32SYS;
    desired.freq = 48000;
    desired.samples = 1024;
    desired.callback = fillAudio;
    desired.silence = 0;
    SDL_OpenAudio(&desired, NULL);

    SDL_PauseAudio(0);

    FILE *fp = fopen(fileName, "rb");
    Uint8 buffer[4096] = {0};

    while (1)
    {
        int readSize = fread(buffer, 1, sizeof(buffer), fp);
        if (readSize != sizeof(buffer))
        {
            fseek(fp, 0, SEEK_SET);
            readSize = fread(buffer, 1, sizeof(buffer), fp);
        }
        SDL_Log("readSize : %d", readSize);
        g_audioData = buffer;
        g_audioDataSize = readSize;
        while (g_audioDataSize > 0)
        {
            SDL_Delay(1);
        }
    }
    
    SDL_CloseAudio();
    SDL_Quit();
    return 0;
}