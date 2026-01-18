#ifndef MAINMENU_H
#define MAINMENU_H

#include <SDL.h>
#include <vector>
#include <string>

class MainMenu {
public:
    MainMenu(SDL_Renderer* ren, const std::string& assetsDir, int maxLevel);
    ~MainMenu();
    int run(); // returns level 0-maxLevel, -1 for kill

private:
    SDL_Renderer* ren;
    std::vector<SDL_Texture*> textures;
    int currentIndex;
    int maxLevel;
};

#endif
