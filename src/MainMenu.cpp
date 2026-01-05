#include "MainMenu.h"
#include <SDL_image.h>
#include <SDL.h>
#include <vector>
#include <string>

MainMenu::MainMenu(SDL_Renderer* ren, const std::string& assetsDir) : ren(ren), currentIndex(0) {
    std::vector<std::string> names = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "mute", "exit", "kill"};
    for (const auto& name : names) {
        std::string path = assetsDir + "menu_glowne_" + name + ".png";
        SDL_Texture* tex = IMG_LoadTexture(ren, path.c_str());
        textures.push_back(tex);
        if (!tex) {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Failed to load %s", path.c_str());
        }
    }
}

MainMenu::~MainMenu() {
    for (auto tex : textures) {
        if (tex) SDL_DestroyTexture(tex);
    }
}

int MainMenu::run() {
    bool running = true;
    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) return -1;
            if (ev.type == SDL_KEYDOWN) {
                if (ev.key.keysym.scancode == SDL_SCANCODE_LEFT || ev.key.keysym.scancode == SDL_SCANCODE_A) {
                    currentIndex = (currentIndex - 1 + textures.size()) % textures.size();
                } else if (ev.key.keysym.scancode == SDL_SCANCODE_RIGHT || ev.key.keysym.scancode == SDL_SCANCODE_D) {
                    currentIndex = (currentIndex + 1) % textures.size();
                } else if (ev.key.keysym.scancode == SDL_SCANCODE_RETURN || ev.key.keysym.scancode == SDL_SCANCODE_RETURN2) {
                    if (currentIndex == 9) { // mute
                        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Menu", "Wyciszono muzykę", nullptr);
                    } else if (currentIndex == 11) { // kill
                        return -1;
                    } else if (currentIndex == 10) { // exit
                        return 0;
                    } else {
                        return currentIndex + 1; // 1-9
                    }
                }
            }
        }

        SDL_RenderClear(ren);
        if (textures[currentIndex]) {
            SDL_RenderCopy(ren, textures[currentIndex], nullptr, nullptr);
        }
        SDL_RenderPresent(ren);
        SDL_Delay(16);
    }
    return -1;
}
