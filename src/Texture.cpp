#include "Texture.h"
#include <SDL.h>
#include <SDL_image.h>
#include <string>
#include <iostream>

Texture::~Texture() {
    if (tex) {
        SDL_DestroyTexture(tex);
        tex = nullptr;
    }
}

bool Texture::load(SDL_Renderer* renderer, const std::string& path) {
    if (!renderer) return false;

    // Destroy any existing texture
    if (tex) {
        SDL_DestroyTexture(tex);
        tex = nullptr;
        w = h = 0;
    }

    SDL_Surface* surf = IMG_Load(path.c_str());
    if (!surf) {
        SDL_Log("IMG_Load failed for %s: %s", path.c_str(), IMG_GetError());
        return false;
    }

    // Convert to a known pixel format with alpha
    SDL_Surface* conv = SDL_ConvertSurfaceFormat(surf, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(surf);
    if (!conv) {
        SDL_Log("SDL_ConvertSurfaceFormat failed for %s: %s", path.c_str(), SDL_GetError());
        return false;
    }

    SDL_Texture* newTex = SDL_CreateTextureFromSurface(renderer, conv);
    if (!newTex) {
        SDL_Log("SDL_CreateTextureFromSurface failed for %s: %s", path.c_str(), SDL_GetError());
        SDL_FreeSurface(conv);
        return false;
    }

    // Set blend mode so alpha is respected
    SDL_SetTextureBlendMode(newTex, SDL_BLENDMODE_BLEND);

    // Query size
    int texW = 0, texH = 0;
    if (SDL_QueryTexture(newTex, nullptr, nullptr, &texW, &texH) != 0) {
        SDL_Log("SDL_QueryTexture failed: %s", SDL_GetError());
    }

    // Store texture and dimensions
    tex = newTex;
    w = texW;
    h = texH;

    SDL_FreeSurface(conv);
    return true;
}

void Texture::draw(SDL_Renderer* renderer, int x, int y, int drawW, int drawH) {
    if (!renderer || !tex) return;

    // If caller passed non-positive sizes, use stored texture size
    int dstW = (drawW > 0) ? drawW : w;
    int dstH = (drawH > 0) ? drawH : h;
    if (dstW <= 0 || dstH <= 0) return;

    SDL_Rect dst{ x, y, dstW, dstH };
    SDL_RenderCopy(renderer, tex, nullptr, &dst);
}