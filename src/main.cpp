// cpp
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <iostream>
#include "Texture.h"
#include "Player.h"
#include "Level.h"
#include "LevelEditor.h"
#include "Menu.h"
#include <algorithm>
#include <cmath>


static void resolvePlayerCollisions(Player& player, const Level& level, int cellW, int cellH) {
    if (cellW <= 0 || cellH <= 0) return;
    if (level.rows <= 0 || level.cols <= 0) return;

    float px = player.x;
    float py = player.y;
    float pw = (float)player.width;
    float ph = (float)player.height;

    int minCol = (int)std::floor(px / cellW);
    int maxCol = (int)std::floor((px + pw - 0.0001f) / cellW);
    int minRow = (int)std::floor(py / cellH);
    int maxRow = (int)std::floor((py + ph - 0.0001f) / cellH);

    minCol = std::max(0, minCol);
    minRow = std::max(0, minRow);
    maxCol = std::min(level.cols - 1, maxCol);
    maxRow = std::min(level.rows - 1, maxRow);

    for (int r = minRow; r <= maxRow; ++r) {
        if (r < 0 || r >= (int)level.grid.size()) continue;
        for (int c = minCol; c <= maxCol; ++c) {
            if (c < 0 || c >= (int)level.grid[r].size()) continue;
            if (level.grid[r][c] == 0) continue; // non-solid

            float tx = (float)(c * cellW);
            float ty = (float)(r * cellH);

            float ix = std::min(px + pw, tx + cellW) - std::max(px, tx);
            float iy = std::min(py + ph, ty + cellH) - std::max(py, ty);

            if (ix > 0.0f && iy > 0.0f) {
                // push out along smaller penetration
                if (ix < iy) {
                    // horizontal resolve: move left or right, keep vertical velocity/state
                    if ((px + pw * 0.5f) < (tx + cellW * 0.5f)) {
                        player.x -= ix;
                    } else {
                        player.x += ix;
                    }
                    px = player.x;
                } else {
                    // vertical resolve: move up or down and adjust vertical velocity/onGround
                    if ((py + ph * 0.5f) < (ty + cellH * 0.5f)) {
                        // player is above tile -> land on it
                        player.y -= iy;
                        player.vy = 0.0f;
                        player.onGround = true;
                    } else {
                        // player is below tile -> hit head
                        player.y += iy;
                        // if moving upward, stop upward velocity; remain not on ground
                        if (player.vy < 0.0f) player.vy = 0.0f;
                        player.onGround = false;
                    }
                    py = player.y;
                }
            }
        }
    }
}

int main(int argc, char* argv[]){
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

    const int WINW = 1024, WINH = 256;
    SDL_Window* win = SDL_CreateWindow("Projekcik", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINW, WINH, SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_SHOWN);
    if(!win){ std::cerr << "CreateWindow failed\n"; IMG_Quit(); SDL_Quit(); return 1; }
    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if(!ren){ std::cerr << "CreateRenderer failed\n"; SDL_DestroyWindow(win); IMG_Quit(); SDL_Quit(); return 1; }

    // build reliable assets directory relative to exe
    char* basePath = SDL_GetBasePath(); // caller must free with SDL_free
    std::string assetsDir;
    if (basePath) {
        assetsDir = std::string(basePath) + "assets/";
        SDL_free(basePath);
    } else {
        assetsDir = "assets/";
    }

    // Load assets using assetsDir
    Texture bgTex;
    bgTex.load(ren, (assetsDir + "poziom_0_tlo.jpg").c_str());

    Texture f1,f2,f3;
    f1.load(ren, (assetsDir + "chodzenie_1.png").c_str());
    f2.load(ren, (assetsDir + "chodzenie_2.png").c_str());
    f3.load(ren, (assetsDir + "chodzenie_3.png").c_str());

    // Abort gracefully if required textures are missing
    if (!bgTex.tex || !f1.tex || !f2.tex || !f3.tex) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Missing assets",
                                 "One or more assets failed to load. Ensure the `assets` folder is next to the executable or adjust the working directory.",
                                 win);
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    Player player;
    player.frames = { &f3, &f2, &f3, &f1 };
    player.width = 32; player.height = 48;
    player.x = 10.f; player.y = 40.f;

    Level level;
    int actualW = 0, actualH = 0;
    SDL_GetWindowSize(win, &actualW, &actualH);
    level.setFrameSize(actualW, actualH);
    level.backgroundPath = assetsDir + "poziom_0_tlo.jpg";
    level.usedAssets = { assetsDir + "chodzenie_1.png", assetsDir + "chodzenie_2.png", assetsDir + "chodzenie_3.png" };

    const float tileScale = 0.5f;

    int editorW = 0, editorH = 0;
    SDL_GetRendererOutputSize(ren, &editorW, &editorH);
    LevelEditor* editor = new LevelEditor(&level, editorW, editorH, tileScale);
    int camX = 0;

    // Menu: provide font path under assets
    Menu menu(ren, (assetsDir + "DejaVuSans.ttf").c_str(), 18);
    menu.addItem("Reload textures", [&](){
        f1.load(ren, (assetsDir + "chodzenie_1.png").c_str());
        f2.load(ren, (assetsDir + "chodzenie_2.png").c_str());
        f3.load(ren, (assetsDir + "chodzenie_3.png").c_str());
        bgTex.load(ren, (assetsDir + "poziom_0_tlo.jpg").c_str());
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Menu", "Textures reloaded", win);
    });
    menu.addItem("Save level", [&](){
        level.saveToZip("level_saved.zip");
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Menu", "Level saved", win);
    });
    menu.addItem("Diagnostics", [&](){
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Menu", "Diagnostics run (WIP)", win);
    });

    bool running = true;
    bool editMode = false;
    Uint64 last = SDL_GetPerformanceCounter();
    while(running){
        Uint64 now = SDL_GetPerformanceCounter();
        double dt = (double)(now - last) / (double)SDL_GetPerformanceFrequency();
        last = now;

        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) { running = false; break; }

            // Window events (resize -> rebuild editor)
            if (ev.type == SDL_WINDOWEVENT && ev.window.event == SDL_WINDOWEVENT_RESIZED) {
                SDL_GetRendererOutputSize(ren, &editorW, &editorH);
                delete editor;
                editor = new LevelEditor(&level, editorW, editorH, tileScale);
                continue;
            }

            // Toggle menu immediately on 'm'
            if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_m) {
                menu.toggle();
                continue;
            }

            // If menu is visible, let it consume the event
            if (menu.visible()) {
                menu.handleEvent(ev);
                continue;
            }

            // Keyboard shortcuts when menu is not active
            if (ev.type == SDL_KEYDOWN) {
                if (ev.key.keysym.scancode == SDL_SCANCODE_E) {
                    editMode = !editMode;
                    continue;
                }
                if (ev.key.keysym.scancode == SDL_SCANCODE_S && (SDL_GetModState() & KMOD_CTRL)) {
                    level.saveToZip("level_saved.zip");
                    continue;
                }
            }

            // Editor mouse handling (map screen -> editor/world coords)
            if (editMode && ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_LEFT) {
                int winW = 0, winH = 0;
                SDL_GetRendererOutputSize(ren, &winW, &winH);

                float sx = (winW > 0) ? (float)editorW / (float)winW : 1.0f;
                float sy = (winH > 0) ? (float)editorH / (float)winH : 1.0f;

                int scaledX = (int)(ev.button.x * sx + 0.5f);
                int scaledY = (int)(ev.button.y * sy + 0.5f);

                int worldX = scaledX + camX;
                int worldY = scaledY;

                if (editor) editor->handleMouse(worldX, worldY);
                continue;
            }

        }

        const Uint8* kb = SDL_GetKeyboardState(nullptr);
        player.update(dt, kb);

        int winW = 0, winH = 0;
        SDL_GetRendererOutputSize(ren, &winW, &winH);

        int cellW = (level.cols > 0) ? std::max(1, (int)(((float)winW / (float)level.cols) * tileScale)) : 0;
        int cellH = (level.rows > 0) ? std::max(1, (int)(((float)winH / (float)level.rows) * tileScale)) : 0;
        int worldW = (cellW > 0) ? (level.cols * cellW) : 0;
        resolvePlayerCollisions(player, level, cellW, cellH);

        if (player.x < 0.f) player.x = 0.f;
        if (player.x + player.width > (float)worldW) player.x = (float)(worldW - player.width);

        int drawW = 0, drawH = 0;
        if (bgTex.tex) {
            int bw = bgTex.w;
            int bh = bgTex.h;
            if (bw > 0 && bh > 0) {
                float scaleW = (float)winW / (float)bw;
                float scaleH = (float)winH / (float)bh;
                float scale = (scaleH > scaleW) ? scaleH : scaleW;
                drawW = (int)(bw * scale + 0.5f);
                drawH = (int)(bh * scale + 0.5f);
            }
        }
        const float parallax = 0.5f;

        int tileCamMax = std::max(0, worldW - winW);
        int bgCamMax = 0;
        if (drawW > winW && parallax > 0.0f) {
            bgCamMax = (int)std::ceil((drawW - winW) / parallax);
        }
        int camMax = std::max(0, std::max(tileCamMax, bgCamMax));
        float playerCenter = player.x + player.width * 0.5f;

        // update persistent camX (do NOT redeclare)
        if (camMax == 0) {
            camX = 0;
        } else if (playerCenter <= winW * 0.5f) {
            camX = 0;
        } else {
            camX = (int)(playerCenter - winW * 0.5f);
            if (camX < 0) camX = 0;
            if (camX > camMax) camX = camMax;
        }

        SDL_SetRenderDrawColor(ren, 50, 50, 80, 255);
        SDL_RenderClear(ren);

        if (bgTex.tex) {
            int bw = bgTex.w;
            int bh = bgTex.h;
            if (bw > 0 && bh > 0) {
                float scaleW = (float)winW / (float)bw;
                float scaleH = (float)winH / (float)bh;
                float scale = (scaleH > scaleW) ? scaleH : scaleW;
                drawW = (int)(bw * scale + 0.5f);
                drawH = (int)(bh * scale + 0.5f);
                int x = (int)(-camX * parallax);
                int minX = winW - drawW;
                if (x < minX) x = minX;
                if (x > 0) x = 0;
                int y = (winH - drawH) / 2;
                bgTex.draw(ren, x, y, drawW, drawH);
            } else {
                bgTex.draw(ren, 0, 0, winW, winH);
            }
        }

        SDL_SetRenderDrawColor(ren, 120, 80, 40, 255);
        for (int r = 0; r < level.rows; ++r) {
            for (int c = 0; c < level.cols; ++c) {
                int cell = 0;
                if (r >= 0 && r < (int)level.grid.size() && c >= 0 && c < (int)level.grid[r].size()) {
                    cell = level.grid[r][c];
                }
                if (cell && cellW > 0 && cellH > 0) {
                    SDL_Rect rect{ c * cellW - camX, r * cellH, cellW, cellH };
                    SDL_RenderFillRect(ren, &rect);
                }
            }
        }

        player.render(ren, camX, 0);

        if(editMode){
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(ren, 0,0,0,150);
            SDL_Rect hud{10,10,220,30};
            SDL_RenderFillRect(ren,&hud);
        }

        menu.render();

        SDL_RenderPresent(ren);
        SDL_Delay(5);
    }

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    delete editor;
    editor = nullptr;
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    return 0;
}