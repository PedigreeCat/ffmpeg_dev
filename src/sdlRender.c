#include "SDL2/SDL.h"

int isExit = 0;
#define EVENT_FRERESH_VIDEO (SDL_USEREVENT + 0x01)

int refreshVideo(void *data)
{
    while (isExit == 0)
    {
        SDL_Event event;
        event.type = EVENT_FRERESH_VIDEO;
        SDL_PushEvent(&event);
        SDL_Delay(40);
    }
}

int main(int argc, const char **argv)
{
    SDL_Init(SDL_INIT_VIDEO);
    int width = 640;
    int height = 360;
    const char *fileName = argv[1];
    uint8_t buffer[width * height * 3 / 2];
    
    SDL_Window *window = SDL_CreateWindow("sdl Demo", 100, 100, width, height, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (window == NULL)
    {
        SDL_Log("create window failed: %s\n", SDL_GetError());
        return -1;
    }

    

    SDL_Renderer * renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == NULL)
    {
        SDL_Log("create renderer failed: %s\n", SDL_GetError());
        goto end;
    }

    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, width, height);
    if (texture == NULL)
    {

    }

    FILE *fp = fopen(fileName, "rb");
    if (fp == NULL)
    {

    }

    SDL_CreateThread(refreshVideo, "refreshVideo", NULL);

    while (1)
    {
        SDL_Event event;
        SDL_WaitEvent(&event);
        if (event.type == SDL_QUIT)
        {
            isExit = 1;
            break;
        }
        else if (event.type == EVENT_FRERESH_VIDEO)
        {
            memset(buffer, 0, sizeof(buffer));
            int readSize = fread(buffer, 1, sizeof(buffer), fp);
            if (readSize != sizeof(buffer))
            {
                fseek(fp, 0, SEEK_SET);
                readSize = fread(buffer, 1, sizeof(buffer), fp);
            }
            SDL_UpdateTexture(texture, NULL, buffer, width);
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, texture, NULL, NULL);
            SDL_RenderPresent(renderer);
        }
    }
    
end:
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}