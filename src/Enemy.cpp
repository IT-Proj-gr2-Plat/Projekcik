#include "Enemy.h"
#include "Texture.h"
#include <SDL.h>
#include <algorithm>

void Enemy::update(double dt, int levelWidth) {
    if (isDead) {
        // Only animate death, no movement
        if (!frames.empty()) {
            frameTime += dt * 1000.0;
            if (frameTime >= frameDelay) {
                curFrame++;
                frameTime = 0;
                if (curFrame >= static_cast<int>(frames.size())) {
                    active = false; // Death animation finished
                }
            }
        }
        return;
    }

    // Move horizontally
    x += vx * (float)dt;

    // Bounce off edges
    if (x <= 0) {
        x = 0;
        vx = -vx;
        facingLeft = true;
    } else if (x + width >= levelWidth) {
        x = levelWidth - width;
        vx = -vx;
        facingLeft = false;
    }

    // Update facing direction based on velocity
    if (vx < 0) facingLeft = false;
    else if (vx > 0) facingLeft = true;

    // Apply gravity
    vy += 1200.f * (float)dt;
    y += vy * (float)dt;

    // Advance animation
    if (!frames.empty()) {
        frameTime += dt * 1000.0;
        if (frameTime >= frameDelay) {
            curFrame = (curFrame + 1) % static_cast<int>(frames.size());
            frameTime = 0;
        }
        // clamp curFrame
        if (curFrame < 0) curFrame = 0;
        if (curFrame >= static_cast<int>(frames.size())) curFrame = static_cast<int>(frames.size()) - 1;
    } else {
        curFrame = 0;
        frameTime = 0;
    }
}

void Enemy::render(SDL_Renderer* r, int camX, int camY, float renderScale) {
    if (!r) return;
    if (frames.empty()) return;

    SDL_Rect dst{
        static_cast<int>((x - camX) * renderScale),
        static_cast<int>((y - height - camY) * renderScale),
        static_cast<int>(width * renderScale),
        static_cast<int>(height * renderScale)
    };

    Texture* tex = frames[curFrame];
    if (tex && tex->tex) {
        SDL_RenderCopyEx(r, tex->tex, nullptr, &dst, 0.0, nullptr, facingLeft ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);
    }
}
