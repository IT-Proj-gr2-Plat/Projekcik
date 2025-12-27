#include "Texture.h"
#include <SDL_image.h>

Texture::~Texture(){
    if(tex) SDL_DestroyTexture(tex);
}

bool Texture::load(SDL_Renderer* r, const std::string& path){
    if(tex){ SDL_DestroyTexture(tex); tex = nullptr; }
    SDL_Surface* s = IMG_Load(path.c_str());
    if(!s) return false;
    tex = SDL_CreateTextureFromSurface(r, s);
    if(tex){ w = s->w; h = s->h; }
    SDL_FreeSurface(s);
    return tex != nullptr;
}

void Texture::draw(SDL_Renderer* r, int x, int y, int w_, int h_){
    if(!tex) return;
    SDL_Rect dst{ x, y, w_ < 0 ? w : w_, h_ < 0 ? h : h_ };
    SDL_RenderCopy(r, tex, nullptr, &dst);
}
