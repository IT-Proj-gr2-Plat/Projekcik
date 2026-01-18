#include "MainMenu.h"
#include <SDL_image.h>
#include <SDL.h>
#include <SDL_mixer.h>
#include <vector>
#include <string>

static bool isMuted = false;

MainMenu::MainMenu(SDL_Renderer* ren, const std::string& assetsDir, int maxLevel) : ren(ren), currentIndex(0), maxLevel(maxLevel) {
    std::vector<std::string> names;
    for (int i = 1; i <= maxLevel; ++i) {
        std::string name = (i == 10) ? "boss" : std::to_string(i);
        names.push_back(name);
    }
    names.push_back("mute");
    names.push_back("exit");
    names.push_back("kill");
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
                    if (currentIndex < maxLevel) {
                        return currentIndex + 1;
                    } else if (currentIndex == maxLevel) { // mute
                        isMuted = !isMuted;
                        if (isMuted) {
                            Mix_VolumeMusic(0);
                        } else {
                            Mix_VolumeMusic(128);
                        }
                        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Menu", isMuted ? "Wyciszono muzykę" : "Włączono muzykę", nullptr);
                    } else { // exit or kill
                        return -1;
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
