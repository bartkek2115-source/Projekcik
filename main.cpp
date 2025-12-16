#include <SDL.h>
#include <iostream>

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << "\n";
        return 1;
    }

    SDL_Window* win = SDL_CreateWindow("Projekcik",
                                       SDL_WINDOWPOS_CENTERED,
                                       SDL_WINDOWPOS_CENTERED,
                                       1920, 1080,
                                       SDL_WINDOW_SHOWN);
    if (!win) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << "\n";
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    if (!ren) {
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }

    bool running = true;
    SDL_Event e;
    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
        }

        SDL_SetRenderDrawColor(ren, 255, 255, 255, 0);
        SDL_RenderClear(ren);
        SDL_RenderPresent(ren);
        SDL_Delay(16);
    }

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}