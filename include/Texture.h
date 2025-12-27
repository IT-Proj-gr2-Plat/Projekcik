#pragma once
#include <SDL.h>
#include <string>

class Texture {
public:
    SDL_Texture* tex = nullptr;
    int w = 0, h = 0;
    Texture() = default;
    ~Texture();
    bool load(SDL_Renderer* r, const std::string& path);
    void draw(SDL_Renderer* r, int x, int y, int w_ = -1, int h_ = -1);
};