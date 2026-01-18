#include "Player.h"
#include "Texture.h"
#include <SDL.h>
#include <algorithm>

void Player::update(double dt, const Uint8* kb){
    const float speed = 420.f;
    float vx = 0.f;
    bool moving = false;
    if(kb[SDL_SCANCODE_LEFT]){ vx -= speed * (float)dt; moving = true; }
    if(kb[SDL_SCANCODE_RIGHT]){ vx += speed * (float)dt; moving = true; }
    if(kb[SDL_SCANCODE_A]){ vx -= speed * (float)dt; moving = true; }
    if(kb[SDL_SCANCODE_D]){ vx += speed * (float)dt; moving = true; }
    this->vx = vx;
    x += vx;
    if(kb[SDL_SCANCODE_SPACE] && onGround){ vy = -450.f; onGround = false; }
    vy += 1200.f * (float)dt;
    y += vy * (float)dt;
    if(y > 900.f){ y = 900.f; vy = 0.f; onGround = true; }

    // Update facing direction
    if (vx < 0) facingLeft = true;
    else if (vx > 0) facingLeft = false;

    if(invulnTimer > 0.0f){
        invulnTimer -= (float)dt;
        if(invulnTimer < 0.0f) invulnTimer = 0.0f;
    }

    if (projectileCooldown > 0.0f) {
        projectileCooldown -= (float)dt;
        if (projectileCooldown < 0.0f) projectileCooldown = 0.0f;
    }

    // Advance animation only when we have frames
    if(!frames.empty()){
        frameTime += dt * 1000.0;
        if(moving){
            if(frameTime >= frameDelay){
                curFrame = (curFrame + 1) % static_cast<int>(frames.size());
                frameTime = 0;
            }
        } else {
            curFrame = 0;
            frameTime = 0;
        }
        // clamp curFrame to valid range
        if(curFrame < 0) curFrame = 0;
        if(curFrame >= static_cast<int>(frames.size())) curFrame = static_cast<int>(frames.size()) - 1;
    } else {
        // reset animation if no frames
        curFrame = 0;
        frameTime = 0;
    }
}

void Player::render(SDL_Renderer* r, int camX, int camY, float renderScale){
    if(!r) return;
    if(frames.empty()) return;
    Texture* t = frames[curFrame];
    if(!t || !t->tex) return;

    SDL_SetTextureBlendMode(t->tex, SDL_BLENDMODE_BLEND);

    int srcW = 0, srcH = 0;
    SDL_QueryTexture(t->tex, nullptr, nullptr, &srcW, &srcH);
    if (srcH == 0) return;

    int baseW = (width > 0) ? width : srcW;
    int baseH = (height > 0) ? height : srcH;

    int destW = (int)(baseW * renderScale + 0.5f);
    int destH = (int)(baseH * renderScale + 0.5f);

    // Treat y as the player's feet (bottom). Subtract base height before rendering.
    int dstX = (int)((x - camX) * renderScale + 0.5f);
    int dstY = (int)((y - camY - baseH) * renderScale + 0.5f);

    SDL_Rect dst{ dstX, dstY, destW, destH };
    SDL_RendererFlip flip = facingLeft ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
    SDL_RenderCopyEx(r, t->tex, nullptr, &dst, 0.0, nullptr, flip);
}