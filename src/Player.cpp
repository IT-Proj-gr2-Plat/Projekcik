#include "Player.h"
#include "Texture.h"
#include <SDL.h>

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

    // Ensure the texture blends with alpha.
    SDL_SetTextureBlendMode(t->tex, SDL_BLENDMODE_BLEND);

    int srcW = 0, srcH = 0;
    SDL_QueryTexture(t->tex, nullptr, nullptr, &srcW, &srcH);
    if (srcH == 0) return;

    // Use explicit player width/height for drawing so sprite matches physics AABB.
    // Fallback to texture size if width/height are not set (>0).
    int destW = (width > 0) ? width : srcW;
    int destH = (height > 0) ? height : srcH;

    // Render using top-left coordinates (player.x, player.y) so physics and rendering align.
    SDL_Rect dst{
        static_cast<int>(x - camX),
        static_cast<int>(y - camY),
        destW,
        destH
    };

    SDL_RenderCopy(r, t->tex, nullptr, &dst);
}