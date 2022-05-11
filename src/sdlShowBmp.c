#include "SDL2/SDL.h"

int main(int argc, const char **argv)
{
    SDL_Init(SDL_INIT_VIDEO);
    const char *fileName = argv[1];
    SDL_Window *window = SDL_CreateWindow("sdl Demo", 100, 100, 640, 480, SDL_WINDOW_SHOWN);
    if (window == NULL)
    {
        SDL_Log("create window failed: %s\n", SDL_GetError());
        return -1;
    }

    SDL_Surface *windowSurface = SDL_GetWindowSurface(window);
    if (windowSurface == NULL)
    {
        SDL_Log("get window surface failed: %s\n", SDL_GetError());
        goto end;
    }

    SDL_Surface *bmpSurface = SDL_LoadBMP(fileName);
    if (bmpSurface == NULL)
    {
        SDL_Log("open file %s failed: %s\n", fileName, SDL_GetError());
        goto end;
    }

    int ret = SDL_BlitSurface(bmpSurface, NULL, windowSurface, NULL);
    if (ret != 0)
    {
        SDL_Log("Blit Surface failed: %s\n", SDL_GetError());
        goto end;
    }
    ret = SDL_UpdateWindowSurface(window);
    if (ret != 0)
    {
        SDL_Log("Update Window Surface failed: %s\n", SDL_GetError());
        goto end;
    }

    while (1)
    {
        SDL_Event event;
        SDL_WaitEvent(&event);
        // SDL_PollEvent(&event);
        if (event.type == SDL_QUIT)
        {
            break;
        }
        SDL_Log("event.type: %d\n", event.type);
    }
    
end:
    if (bmpSurface)
    {
        SDL_FreeSurface(bmpSurface);
        bmpSurface = NULL;
    }
    // SDL_Delay(5 * 1000);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}