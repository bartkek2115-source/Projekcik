#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <iostream>
#include "Texture.h"
#include "Player.h"
#include "Level.h"
#include "LevelEditor.h"
#include "Menu.h"
#include "MainMenu.h"
#include <algorithm>
#include <cmath>
#include <string>


static void resolvePlayerCollisions(Player& player, Level& level, int cellW, int cellH) {
    if (cellW <= 0 || cellH <= 0) return;
    if (level.rows <= 0 || level.cols <= 0) return;

    const float eps = 0.0001f;

    // Physics: player.x is left, player.y is _feet_ (bottom).
    float px = player.x;
    float pw = static_cast<float>(player.width);
    float top = player.y - static_cast<float>(player.height);
    float ph = static_cast<float>(player.height);

    int minCol = (int)std::floor(px / cellW);
    int maxCol = (int)std::floor((px + pw - eps) / cellW);
    int minRow = (int)std::floor(top / cellH);
    int maxRow = (int)std::floor((top + ph - eps) / cellH);

    minCol = std::max(0, minCol);
    minRow = std::max(0, minRow);
    maxCol = std::min(level.cols - 1, maxCol);
    maxRow = std::min(level.rows - 1, maxRow);

    for (int r = minRow; r <= maxRow; ++r) {
        if (r < 0 || r >= (int)level.grid.size()) continue;
        for (int c = minCol; c <= maxCol; ++c) {
            if (c < 0 || c >= (int)level.grid[r].size()) continue;

            int cell = level.grid[r][c]; // 0=empty,1=solid,2=damaging,3=pickup
            if (cell == 0) continue; // non-solid

            float tx = static_cast<float>(c * cellW);
            float ty = static_cast<float>(r * cellH);

            float ix = std::min(px + pw, tx + cellW) - std::max(px, tx);
            float iy = std::min(top + ph, ty + cellH) - std::max(top, ty);

            if (ix > 0.0f && iy > 0.0f) {
                if (cell == 3) {
                    player.score += 10;
                    level.grid[r][c] = 0; // remove pickup
                    continue;
                }

                bool isDamaging = (cell == 2);

                // Resolve along smaller penetration (push player out)
                if (ix < iy) {
                    // horizontal push
                    if (px + pw * 0.5f < tx + cellW * 0.5f) {
                        // push left
                        px -= ix;
                    } else {
                        // push right
                        px += ix;
                    }
                    // apply immediate horizontal correction
                    player.x = px;
                } else {
                    // vertical push
                    if (top + ph * 0.5f < ty + cellH * 0.5f) {
                        // collision from above -> place player on top of tile
                        top = ty - ph;
                        player.vy = 0.0f;
                        player.onGround = true;
                    } else {
                        // collision from below -> push player down (head hit)
                        top += iy;
                        if (player.vy < 0.0f) player.vy = 0.0f;
                    }
                    // apply immediate vertical correction
                    player.y = top + ph;
                }

                // Handle damage
                if (isDamaging && player.invulnTimer <= 0.0f) {
                    player.health -= 1;
                    player.invulnTimer = player.invuln;
                    if (player.health < 0) player.health = 0;
                }
            }
        }
    }

    // Ensure the resolved values are applied
    player.x = px;
    player.y = top + ph;
}

int main(int argc, char* argv[]) {
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0){
        std::cerr << "SDL_Init Error: " << SDL_GetError() << "\n";
        return 1;
    }

    int imgFlags = IMG_INIT_JPG | IMG_INIT_PNG;
    if((IMG_Init(imgFlags) & imgFlags) != imgFlags){
        std::cerr << "IMG_Init failed: " << IMG_GetError() << "\n";
    }

    if(TTF_Init() != 0){
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "TTF_Init failed: %s", TTF_GetError());
    }

    // Scaling fix (WIP)
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

    const int WINH = 288, WINW = 512;
    SDL_Window* win = SDL_CreateWindow("Projekcik", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINW, WINH, SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_SHOWN);
    if(!win){ std::cerr << "CreateWindow failed\n"; IMG_Quit(); SDL_Quit(); return 1; }
    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if(!ren){ std::cerr << "CreateRenderer failed\n"; SDL_DestroyWindow(win); IMG_Quit(); SDL_Quit(); return 1; }

    // keep logical game coords at WINW x WINH even in fullscreen
    SDL_RenderSetLogicalSize(ren, WINW, WINH);

    // asset path setup
    char* basePath = SDL_GetBasePath();
    std::string assetsDir;
    if (basePath) {
        assetsDir = std::string(basePath) + "assets/";
        SDL_free(basePath);
    } else {
        assetsDir = "assets/";
    }

    TTF_Font* hudFont = nullptr;
    std::string hudFontPath = (assetsDir + "DejaVuSans.ttf");
    hudFont = TTF_OpenFont(hudFontPath.c_str(), 24); // larger for game over screens
    if(!hudFont){
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "HUD font not opened: %s", TTF_GetError());
        hudFont = nullptr;
    }

    // Main game loop
    while (true) {
        // Show main menu
        MainMenu mainMenu(ren, assetsDir);
        int selectedLevel = mainMenu.run();
        if (selectedLevel == -1) break; // kill

        std::string bgFile = "poziom_" + std::to_string(selectedLevel) + "_tlo.jpg";

        // Load assets using assetsDir
        Texture bgTex;
        bgTex.load(ren, (assetsDir + bgFile).c_str());

        Texture f1,f2,f3;
        f1.load(ren, (assetsDir + "chodzenie_1.png").c_str());
        f2.load(ren, (assetsDir + "chodzenie_2.png").c_str());
        f3.load(ren, (assetsDir + "chodzenie_3.png").c_str());

        // Abort gracefully if required textures are missing
        if (!bgTex.tex || !f1.tex || !f2.tex || !f3.tex) {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Missing assets",
                                     "One or more assets failed to load. Ensure the `assets` folder is next to the executable or adjust the working directory.",
                                     win);
            continue; // back to menu
        }

        // Use logical WINW/WINH for level/frame sizing and rendering math
        Level level;
        level.setFrameSize(WINW, WINH);
        level.cols = 156; // map size
        level.grid.assign(level.rows, std::vector<int>(level.cols, 0));
        SDL_Log("DBG: level frame size set to %dx%d", WINW, WINH);
        int groundRow = level.rows - 2;
        if (groundRow >= 0) {
            for (int c = 0; c < level.cols; ++c) {
                level.grid[groundRow][c] = 1; // solid ground
            }
        }

        level.setBackgroundTexture(bgTex.tex);
        level.setBackgroundRepeat(false); // scroll once
        level.setScrollSpeed(0.0f); // no auto-scroll
        level.setParallax(0.25f); // parallax
        level.setBackgroundMaxSpeed(50.0f); // max 50 px/sec

        Player player;
        player.frames = { &f3, &f2, &f3, &f1 };
        player.width = 32; player.height = 48;
        player.x = 10.f;

        // Place player on ground initially
        int levelH = level.rows * 32; // base tile = 32
        player.y = static_cast<float>(std::max(0, levelH - player.height)); // put player on bottom of level
        player.onGround = true;
        player.vy = 0.0f;

        level.backgroundPath = assetsDir + bgFile;
        level.usedAssets = { assetsDir + "chodzenie_1.png", assetsDir + "chodzenie_2.png", assetsDir + "chodzenie_3.png" };

        const float editorTileScale = 1.0f;   // used only by LevelEditor
        const float renderTileScale = 1.0f;   // used for runtime drawing / player size scaling
        const int baseTilePixels = 32;        // physical base tile size (used for collision/camera)

        // create editor
        LevelEditor* editor = new LevelEditor(&level, WINW, WINH, editorTileScale, baseTilePixels);
        float camX = 0.0f;
        float editorCamX = 0.0f;

        // Menu setup
        Menu menu(ren, (assetsDir + "DejaVuSans.ttf").c_str(), 18);
        menu.addItem("Reload textures", [&](){
            f1.load(ren, (assetsDir + "chodzenie_1.png").c_str());
            f2.load(ren, (assetsDir + "chodzenie_2.png").c_str());
            f3.load(ren, (assetsDir + "chodzenie_3.png").c_str());
            bgTex.load(ren, (assetsDir + "poziom_0_tlo.jpg").c_str());
            level.setBackgroundTexture(bgTex.tex);
            level.setBackgroundRepeat(false);
            level.setScrollSpeed(0.0f);
            level.setParallax(0.25f);
            level.setBackgroundMaxSpeed(50.0f);
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Menu", "Textures reloaded", win);
    });
        menu.addItem("Save level", [&](){
            level.saveToZip("level_saved.zip");
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Menu", "Level saved", win);
        });

        bool running = true;
        bool editMode = false;
        bool playerLost = false;
        bool playerWon = false;
        float fade = 0.0f;
        Uint64 last = SDL_GetPerformanceCounter();

        // Game loop
        while(running) {
            Uint64 now = SDL_GetPerformanceCounter();
            double dt = (double)(now - last) / (double)SDL_GetPerformanceFrequency();
            last = now;

            SDL_Event ev;
            while (SDL_PollEvent(&ev)) {
                if (ev.type == SDL_QUIT) { running = false; break; }

                if (ev.type == SDL_WINDOWEVENT && ev.window.event == SDL_WINDOWEVENT_RESIZED) {
                    // Recreate editor on window size change
                    delete editor;
                    editor = new LevelEditor(&level, WINW, WINH, editorTileScale, baseTilePixels);
                    continue;
                }

                if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_m) {
                    menu.toggle();
                    continue;
                }

                if (menu.visible()) {
                    menu.handleEvent(ev);
                    continue;
                }

                if (ev.type == SDL_KEYDOWN) {
                    if (ev.key.keysym.scancode == SDL_SCANCODE_E) { editMode = !editMode; if (editMode) editorCamX = camX; continue; }
                    if (ev.key.keysym.scancode == SDL_SCANCODE_S && (SDL_GetModState() & KMOD_CTRL)) {
                        level.saveToZip("level_saved.zip");
                        continue;
                    }
                    if (ev.key.keysym.scancode == SDL_SCANCODE_F11) {
                        Uint32 flags = SDL_GetWindowFlags(win);
                        if (flags & SDL_WINDOW_FULLSCREEN_DESKTOP) {
                            SDL_SetWindowFullscreen(win, 0);
                        } else {
                            SDL_SetWindowFullscreen(win, SDL_WINDOW_FULLSCREEN_DESKTOP);
                        }
                        continue;
                    }
                }

                if (editMode && ev.type == SDL_KEYDOWN) {
                    if (ev.key.keysym.scancode == SDL_SCANCODE_LEFT) editorCamX -= 32.0f;
                    if (ev.key.keysym.scancode == SDL_SCANCODE_RIGHT) editorCamX += 32.0f;
                    float maxCam = std::max(0.0f, (float)(level.cols * baseTilePixels) - (float)WINW / renderTileScale);
                    editorCamX = std::max(0.0f, std::min(editorCamX, maxCam));
                    continue;
                }

    if (editMode && ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_LEFT) {
        int winMouseX_evt = ev.button.x;
        int winMouseY_evt = ev.button.y;
        int winMouseX_state = 0, winMouseY_state = 0;
        SDL_GetMouseState(&winMouseX_state, &winMouseY_state);

        // Window size in window coordinates
        int winW_win = 0, winH_win = 0;
        SDL_GetWindowSize(win, &winW_win, &winH_win);
        if (winW_win <= 0 || winH_win <= 0) continue;

        // Renderer output size in pixels
        int outW_pixels = 0, outH_pixels = 0;
        SDL_GetRendererOutputSize(ren, &outW_pixels, &outH_pixels);
        if (outW_pixels <= 0 || outH_pixels <= 0) continue;

        // Logical size
        int logicalW = 0, logicalH = 0;
        SDL_RenderGetLogicalSize(ren, &logicalW, &logicalH);
        if (logicalW <= 0 || logicalH <= 0) continue;

        float lx = static_cast<float>(winMouseX_state) * static_cast<float>(logicalW) / static_cast<float>(winW_win);
        float ly = static_cast<float>(winMouseY_state) * static_cast<float>(logicalH) / static_cast<float>(winH_win);

        float editorScale = 1.0f / editorTileScale;
        float mx_editor = lx * editorScale;
        float my_editor = ly * editorScale;

        // Compute floating camera offset in editor pixel-space (avoid rounding)
        float camX_editor_f = camX * editorScale;

        // Pass float logical coordinates to editor for precise mapping
        editor->handleMouse(mx_editor, my_editor, camX_editor_f);
        continue;
            }

            // frame update & render
            const Uint8* kb = SDL_GetKeyboardState(nullptr);
            if (!editMode && !playerLost && !playerWon) player.update(dt, kb);

            if (editMode) {
                if (kb[SDL_SCANCODE_LEFT]) editorCamX -= 2000.0f * dt;
                if (kb[SDL_SCANCODE_RIGHT]) editorCamX += 2000.0f * dt;
                float maxCam = std::max(0.0f, (float)(level.cols * baseTilePixels) - (float)WINW / renderTileScale);
                editorCamX = std::max(0.0f, std::min(editorCamX, maxCam));
            }

            // Use logical size for drawing / camera math
            int winW = WINW;
            int winH = WINH;

            int renderCellW = std::max(1, (int)(baseTilePixels * renderTileScale + 0.5f));
            int renderCellH = renderCellW;
            int physCellW = baseTilePixels;
            int physCellH = baseTilePixels;
            float renderScale = (float)renderCellW / (float)physCellW;

            int levelW = level.cols * physCellW;
            int levelH_now = level.rows * physCellH;

            int worldW = std::max(levelW, winW);
            int worldH = std::max(levelH_now, winH);

            if (!editMode && !playerLost && !playerWon) {
                resolvePlayerCollisions(player, level, physCellW , physCellH);

                // Check for game over conditions
                if (player.health <= 0) {
                    playerLost = true;
                    fade = 0.0f;
                    running = false;
                }
                if (player.x >= levelW - player.width) {
                    playerWon = true;
                    running = false;
                }
            }

            // clamp player to level bounds (physics units)
            if (levelW > 0) {
                if (player.x < 0.f) player.x = 0.f;
                float maxPlayerX = (float)std::max(0, levelW - player.width);
                if (player.x > maxPlayerX) player.x = maxPlayerX;
            }
            if (levelH_now > 0) {
                if (player.y < 0.f) player.y = 0.f;
                float maxPlayerY = (float)std::max(0, levelH_now - player.height);
                if (player.y > maxPlayerY) { player.y = maxPlayerY; player.onGround = true; player.vy = 0.f; }
            }

            // Camera: center on player in physics units, clamp to level bounds
            float camTarget = player.x - ( (float)winW / (2.0f * renderScale) );
            float maxCam = std::max(0.0f, (float)(levelW) - (float)winW / renderScale);
            camX = std::max(0.0f, std::min(camTarget, maxCam));

            float levelWorldW = static_cast<float>(level.cols * physCellW);
            float camWidthWorld = static_cast<float>(winW) / renderScale;
            float maxCamWorld = std::max(0.0f, levelWorldW - camWidthWorld);

            float playerCenter = player.x + (player.width * 0.5f);
            float camTargetWorld = playerCenter - (camWidthWorld * 0.5f);
            float camX_world = std::max(0.0f, std::min(maxCamWorld, camTargetWorld));

            // keep camX in sync
            camX = camX_world;

            if (editMode) camX = editorCamX;

            // Compute floating render-space camera for background rendering
            float camX_render_f = camX * renderScale;
            float camMax_render_f = maxCamWorld * renderScale;

            // Integer camera for rendering tiles/player
            int camX_render = static_cast<int>(std::lround(camX_render_f));
            int camMax_render = static_cast<int>(std::lround(camMax_render_f));
            if (camX_render < 0) camX_render = 0;
            if (camMax_render < 0) camMax_render = 0;
            if (camX_render > camMax_render) camX_render = camMax_render;

            // Pass floating camera values to level background
            level.setBackgroundOffsetFromCamera(camX_render_f, camMax_render_f, (float)dt);
            level.updateBackground((float)dt);


            // Clear and draw: background, tiles, player, HUD
            SDL_SetRenderDrawColor(ren, 50, 50, 80, 255);
            SDL_RenderClear(ren);

            // Level background
            level.renderBackground(ren);


            // draw tiles using camX_render
            for (int r = 0; r < level.rows; ++r) {
                for (int c = 0; c < level.cols; ++c) {
                    int cell = 0;
                    if (r >= 0 && r < (int)level.grid.size() && c >= 0 && c < (int)level.grid[r].size()) {
                        cell = level.grid[r][c];
                    }
                    if (cell == 0) continue;

                    int tileX_render = c * renderCellW - camX_render;
                    int tileY_render = r * renderCellH;
                    SDL_Rect dst{ tileX_render, tileY_render, renderCellW, renderCellH };

                    switch (cell) {
                        case 1:
                            SDL_SetRenderDrawColor(ren, 128, 128, 128, 255);
                            SDL_RenderFillRect(ren, &dst);
                            break;
                        case 2:
                            SDL_SetRenderDrawColor(ren, 160, 40, 40, 255);
                            SDL_RenderFillRect(ren, &dst);
                            break;
                        case 3:
                            SDL_SetRenderDrawColor(ren, 200, 200, 60, 255);
                            SDL_RenderFillRect(ren, &dst);
                            break;
                        default:
                            SDL_SetRenderDrawColor(ren, 100, 100, 100, 255);
                            SDL_RenderFillRect(ren, &dst);
                            break;
                    }
                }
            }

            // render player once using same camX_render
            player.render(ren, camX_render, 0, renderScale);

            // HUD/menu rendering
            menu.render();

            if (!editMode) {
                SDL_Color color = {0, 0, 0, 255};
                std::string scoreText = "Punkty: " + std::to_string(player.score);
                std::string healthText = "HP: " + std::to_string(player.health);

                SDL_Surface* surf1 = TTF_RenderText_Blended(hudFont, scoreText.c_str(), color);
                if (surf1) {
                    SDL_Texture* tex1 = SDL_CreateTextureFromSurface(ren, surf1);
                    if (tex1) {
                        int w, h;
                        SDL_QueryTexture(tex1, nullptr, nullptr, &w, &h);
                        SDL_Rect dst = {WINW - w - 10, 10, w, h};
                        SDL_RenderCopy(ren, tex1, nullptr, &dst);
                        SDL_DestroyTexture(tex1);
                    }
                    SDL_FreeSurface(surf1);
                }

                SDL_Surface* surf2 = TTF_RenderText_Blended(hudFont, healthText.c_str(), color);
                if (surf2) {
                    SDL_Texture* tex2 = SDL_CreateTextureFromSurface(ren, surf2);
                    if (tex2) {
                        int w, h;
                        SDL_QueryTexture(tex2, nullptr, nullptr, &w, &h);
                        SDL_Rect dst = {10, 10, w, h};
                        SDL_RenderCopy(ren, tex2, nullptr, &dst);
                        SDL_DestroyTexture(tex2);
                    }
                    SDL_FreeSurface(surf2);
                }
            }

            if (editMode) {
                SDL_Color color = {0, 0, 0, 255};
                SDL_Surface* surf = TTF_RenderText_Blended(hudFont, "Edytor: strza\u0142ki - ruch, lewy myszki - klocek (cykluje warto\u015Bciami)", color);
                if (surf) {
                    SDL_Texture* tex = SDL_CreateTextureFromSurface(ren, surf);
                    if (tex) {
                        int w, h;
                        SDL_QueryTexture(tex, nullptr, nullptr, &w, &h);
                        SDL_Rect dst = {10, 10, w, h};
                        SDL_RenderCopy(ren, tex, nullptr, &dst);
                        SDL_DestroyTexture(tex);
                    }
                    SDL_FreeSurface(surf);
                }
            }

            // Render game over screens
            if (playerLost) {
                fade += (float)dt * 200.0f; // fade in
                if (fade > 255.0f) fade = 255.0f;

                SDL_SetRenderDrawColor(ren, 0, 0, 0, (Uint8)fade);
                SDL_RenderFillRect(ren, nullptr);

                SDL_Color color = {255, 0, 0, 255};
                SDL_Surface* surf = TTF_RenderText_Blended(hudFont, "Przegra\u0142e\u015B", color);
                if (surf) {
                    SDL_Texture* tex = SDL_CreateTextureFromSurface(ren, surf);
                    if (tex) {
                        int w, h;
                        SDL_QueryTexture(tex, nullptr, nullptr, &w, &h);
                        SDL_Rect dst = {WINW / 2 - w / 2, WINH / 2 - h / 2, w, h};
                        SDL_RenderCopy(ren, tex, nullptr, &dst);
                        SDL_DestroyTexture(tex);
                    }
                    SDL_FreeSurface(surf);
                }
            } else if (playerWon) {
                SDL_SetRenderDrawColor(ren, 102, 51, 153, 255);
                SDL_RenderFillRect(ren, nullptr);

                SDL_Color color = {255, 215, 0, 255};
                SDL_Surface* surf = TTF_RenderText_Blended(hudFont, "Wygrałeś", color);
                if (surf) {
                    SDL_Texture* tex = SDL_CreateTextureFromSurface(ren, surf);
                    if (tex) {
                        int w, h;
                        SDL_QueryTexture(tex, nullptr, nullptr, &w, &h);
                        SDL_Rect dst = {WINW / 2 - w / 2, WINH / 2 - h / 2, w, h};
                        SDL_RenderCopy(ren, tex, nullptr, &dst);
                        SDL_DestroyTexture(tex);
                    }
                    SDL_FreeSurface(surf);
                }
            }

            SDL_RenderPresent(ren);
            SDL_Delay(5);
        }

        // Wait for enter to return to menu
        bool waiting = true;
        while (waiting) {
            SDL_Event ev;
            while (SDL_PollEvent(&ev)) {
                if (ev.type == SDL_QUIT) { waiting = false; break; }
                if (ev.type == SDL_KEYDOWN && ev.key.keysym.scancode == SDL_SCANCODE_RETURN) {
                    waiting = false;
                    break;
                }
            }

            // Re-render the last screen
            if (playerLost) {
                SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
                SDL_RenderFillRect(ren, nullptr);
                SDL_Color color = {255, 0, 0, 255};
                SDL_Surface* surf = TTF_RenderText_Blended(hudFont, "Przegra\u0142e\u015B", color);
                if (surf) {
                    SDL_Texture* tex = SDL_CreateTextureFromSurface(ren, surf);
                    if (tex) {
                        int w, h;
                        SDL_QueryTexture(tex, nullptr, nullptr, &w, &h);
                        SDL_Rect dst = {WINW / 2 - w / 2, WINH / 2 - h / 2, w, h};
                        SDL_RenderCopy(ren, tex, nullptr, &dst);
                        SDL_DestroyTexture(tex);
                    }
                    SDL_FreeSurface(surf);
                }
            } else if (playerWon) {
                SDL_SetRenderDrawColor(ren, 102, 51, 153, 255);
                SDL_RenderFillRect(ren, nullptr);
                SDL_Color color = {255, 215, 0, 255};
                SDL_Surface* surf = TTF_RenderText_Blended(hudFont, "Wygrałeś", color);
                if (surf) {
                    SDL_Texture* tex = SDL_CreateTextureFromSurface(ren, surf);
                    if (tex) {
                        int w, h;
                        SDL_QueryTexture(tex, nullptr, nullptr, &w, &h);
                        SDL_Rect dst = {WINW / 2 - w / 2, WINH / 2 - h / 2, w, h};
                        SDL_RenderCopy(ren, tex, nullptr, &dst);
                        SDL_DestroyTexture(tex);
                    }
                    SDL_FreeSurface(surf);
                }
            }

            SDL_RenderPresent(ren);
            SDL_Delay(16);
        }

        // Cleanup for this level
        delete editor;
        editor = nullptr;
    }

    // cleanup
    if(hudFont) TTF_CloseFont(hudFont);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    return 0;
}
