#include "Player.h"

void Player::update(double dt, const Uint8* kb){
    const float speed = 220.f;
    bool moving = false;
    if(kb[SDL_SCANCODE_LEFT]){ x -= speed * (float)dt; moving = true; }
    if(kb[SDL_SCANCODE_RIGHT]){ x += speed * (float)dt; moving = true; }
    if(kb[SDL_SCANCODE_SPACE] && onGround){ vy = -450.f; onGround = false; }
    vy += 1200.f * (float)dt;
    y += vy * (float)dt;
    if(y > 400.f){ y = 400.f; vy = 0.f; onGround = true; }

    frameTime += dt * 1000.0;
    if(moving){
        if(frameTime >= frameDelay){
            curFrame = (curFrame + 1) % (int)frames.size();
            frameTime = 0;
        }
    } else {
        curFrame = 0;
    }
}

void Player::render(SDL_Renderer* r, int camX, int camY){
    if(frames.empty()) return;
    Texture* t = frames[curFrame];
    if(!t || !t->tex) return;
    SDL_Rect dst{ (int)(x - camX), (int)(y - camY), width, height };
    SDL_RenderCopy(r, t->tex, nullptr, &dst);
}
