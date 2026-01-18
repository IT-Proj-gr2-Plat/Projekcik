#pragma once
#include "Texture.h"
#include <vector>
#include <SDL.h>

class Enemy {
public:
    float x = 200.f, y = 800.f;
    float vx = 50.f; // horizontal speed
    float vy = 0.f;
    bool onGround = false;
    bool facingLeft = false;
    std::vector<Texture*> frames;
    int curFrame = 0;
    double frameTime = 0;
    double frameDelay = 200.0; // ms per frame
    int width = 32, height = 32;
    bool active = true;
    bool isDead = false;

    void update(double dt, int levelWidth);
    void render(SDL_Renderer* r, int camX, int camY, float renderScale = 1.0f);
};
