#include "SDL2/SDL.h"

int main()
{
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window *window = SDL_CreateWindow("sdl Demo", 100, 100, 640, 480, SDL_WINDOW_SHOWN);
    if (window == NULL)
    {
        SDL_Log("create window failed: %s\n", SDL_GetError());
        return -1;
    }

    while (1)
    {
        SDL_Event event;
        // SDL_WaitEvent(&event);
        SDL_PollEvent(&event);
        if (event.type == SDL_QUIT)
        {
            break;
        }
        SDL_Log("event.type: %d\n", event.type);
    }

    // SDL_Delay(5 * 1000);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}