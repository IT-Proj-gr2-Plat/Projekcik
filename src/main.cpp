#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <iostream>
#include "Texture.h"
#include "Player.h"
#include "Level.h"
#include "LevelEditor.h"
#include "Menu.h"

int main(int argc, char* argv[]){
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0){
        std::cerr << "SDL_Init Error: " << SDL_GetError() << "\n";
        return 1;
    }

    int imgFlags = IMG_INIT_JPG | IMG_INIT_PNG;
    if((IMG_Init(imgFlags) & imgFlags) != imgFlags){
        std::cerr << "IMG_Init failed: " << IMG_GetError() << "\n";
        // continue; some apps prefer to continue without image support
    }

    if(TTF_Init() != 0){
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "TTF_Init failed: %s", TTF_GetError());
        // Menu will detect missing font and degrade gracefully
    }

    const int WINW = 1280, WINH = 720;
    SDL_Window* win = SDL_CreateWindow("Projekcik", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINW, WINH, SDL_WINDOW_SHOWN);
    if(!win){ std::cerr << "CreateWindow failed\n"; IMG_Quit(); SDL_Quit(); return 1; }
    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if(!ren){ std::cerr << "CreateRenderer failed\n"; SDL_DestroyWindow(win); IMG_Quit(); SDL_Quit(); return 1; }

    // Load assets
    Texture bgTex;
    bgTex.load(ren, "assets/poziom_0_tlo.png");

    Texture f1,f2,f3;
    f1.load(ren, "assets/chodzenie_1.png");
    f2.load(ren, "assets/chodzenie_2.png");
    f3.load(ren, "assets/chodzenie_3.png");

    Player player;
    player.frames = { &f3, &f2, &f3, &f1 };
    player.width = 64; player.height = 64;
    player.x = 100.f; player.y = 400.f;

    Level level;
    level.backgroundPath = "assets/poziom_0_tlo.png";
    level.usedAssets = {"assets/chodzenie_1.png","assets/chodzenie_2.png","assets/chodzenie_3.png"};

    LevelEditor editor(&level, WINW, WINH);

    // Menu: provide font path under assets; Menu implementation should handle missing font
    Menu menu(ren, "assets/DejaVuSans.ttf", 18);
    menu.addItem("Reload textures", [&](){
        // basic reload: re-load the three frames and background
        f1.load(ren, "assets/chodzenie_1.png");
        f2.load(ren, "assets/chodzenie_2.png");
        f3.load(ren, "assets/chodzenie_3.png");
        bgTex.load(ren, "assets/poziom_0_tlo.png");
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
        while(SDL_PollEvent(&ev)){
            if(ev.type == SDL_QUIT){ running = false; break; }

            // Toggle menu with 'M'
            if(ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_m){
                menu.toggle();
                continue;
            }

            // If menu visible, let it handle input and skip game input
            if(menu.visible()){
                menu.handleEvent(ev);
                continue;
            }

            if(ev.type == SDL_KEYDOWN){
                if(ev.key.keysym.scancode == SDL_SCANCODE_E) editMode = !editMode;
                if(ev.key.keysym.scancode == SDL_SCANCODE_S && (SDL_GetModState() & KMOD_CTRL)){
                    level.saveToZip("level_saved.zip");
                }
            } else if(editMode && ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_LEFT){
                editor.handleMouse(ev.button.x, ev.button.y);
            }
        }

        const Uint8* kb = SDL_GetKeyboardState(nullptr);
        player.update(dt, kb);

        int camX = (int)(player.x) - WINW/2 + player.width/2;
        if(camX < 0) camX = 0;

        SDL_SetRenderDrawColor(ren, 50, 50, 80, 255);
        SDL_RenderClear(ren);

        if(bgTex.tex){
            int bw = bgTex.w, bh = bgTex.h;
            float parallax = 0.5f;
            int offset = (int)(-camX * parallax) % bw;
            if(offset > 0) offset -= bw;
            for(int x = offset; x < WINW; x += bw){
                bgTex.draw(ren, x, 0, bw, WINH);
            }
        }

        int cellH = WINH / level.rows;
        SDL_SetRenderDrawColor(ren, 120, 80, 40, 255);
        for(int r=0;r<level.rows;r++){
            for(int c=0;c<level.cols;c++){
                if(level.grid[r*level.cols + c]){
                    SDL_Rect rect{ c*cellH - camX/4, r*cellH, cellH, cellH };
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

        // render menu on top if visible
        menu.render();

        SDL_RenderPresent(ren);
        SDL_Delay(5);
    }

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    return 0;
}