#include "Menu.h"
#include <SDL.h>
#include <SDL_ttf.h>

Menu::Menu(SDL_Renderer* renderer, const char* fontPath, int fontSize)
: renderer_(renderer)
{
    font_ = TTF_OpenFont(fontPath, fontSize);
    if(!font_){
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "TTF_OpenFont failed: %s", TTF_GetError());
    }
}

Menu::~Menu(){
    destroyTextures();
    if(font_) TTF_CloseFont(font_);
}

void Menu::createLabelTexture(Item &it){
    if(!renderer_ || !font_) return;
    if(it.tex) SDL_DestroyTexture(it.tex);
    SDL_Surface* surf = TTF_RenderUTF8_Blended(font_, it.label.c_str(), textCol_);
    if(!surf){
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "TTF_RenderUTF8_Blended failed: %s", TTF_GetError());
        return;
    }
    it.tex = SDL_CreateTextureFromSurface(renderer_, surf);
    it.texW = surf->w;
    it.texH = surf->h;
    SDL_FreeSurface(surf);
}

void Menu::destroyTextures(){
    for(auto &it : items_){
        if(it.tex){ SDL_DestroyTexture(it.tex); it.tex = nullptr; }
    }
}

void Menu::addItem(const std::string &label, std::function<void()> cb){
    Item it;
    it.label = label;
    it.cb = cb;
    createLabelTexture(it);
    items_.push_back(it);
    if(selected_ >= items_.size()) selected_ = 0;
}

void Menu::toggle(){
    visible_ = !visible_;
    if(selected_ >= items_.size()) selected_ = 0;
}

bool Menu::visible() const { return visible_; }

void Menu::handleEvent(const SDL_Event &e){
    if(!visible_) return;
    if(e.type == SDL_KEYDOWN){
        switch(e.key.keysym.sym){
            case SDLK_UP:
                if(!items_.empty()) selected_ = (selected_ + items_.size() - 1) % items_.size();
                break;
            case SDLK_DOWN:
                if(!items_.empty()) selected_ = (selected_ + 1) % items_.size();
                break;
            case SDLK_RETURN:
            case SDLK_KP_ENTER:
                if(!items_.empty() && items_[selected_].cb) items_[selected_].cb();
                visible_ = false;
                break;
            case SDLK_ESCAPE:
                visible_ = false;
                break;
            default: break;
        }
    } else if(e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT){
        int mx = e.button.x, my = e.button.y;
        int left = x_ - padding_;
        int top = y_ - padding_;
        int total_h = (int)items_.size() * item_h_ + padding_*2;
        int right = left + w_ + padding_*2;
        int bottom = top + total_h;
        if(mx >= left && mx <= right && my >= top && my <= bottom){
            int idx = (my - top - padding_) / item_h_;
            if(idx >= 0 && idx < (int)items_.size()){
                if(items_[idx].cb) items_[idx].cb();
                visible_ = false;
            }
        }
    }
}

void Menu::render(){
    if(!visible_ || !renderer_) return;
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);

    int total_h = (int)items_.size() * item_h_;
    SDL_Rect bgrect{ x_ - padding_, y_ - padding_, w_ + padding_*2, total_h + padding_*2 };

    // background
    SDL_SetRenderDrawColor(renderer_, bg_.r, bg_.g, bg_.b, bg_.a);
    SDL_RenderFillRect(renderer_, &bgrect);

    // border
    SDL_SetRenderDrawColor(renderer_, border_.r, border_.g, border_.b, border_.a);
    SDL_RenderDrawRect(renderer_, &bgrect);

    // items
    for(size_t i=0;i<items_.size();++i){
        SDL_Rect itemRect{ x_, y_ + (int)i * item_h_ + padding_, w_, item_h_ };
        if(i == selected_){
            SDL_SetRenderDrawColor(renderer_, sel_.r, sel_.g, sel_.b, sel_.a);
            SDL_RenderFillRect(renderer_, &itemRect);
        }
        // draw label texture if present
        Item &it = items_[i];
        if(it.tex){
            SDL_Rect dst{
                x_ + 8,
                itemRect.y + (itemRect.h - it.texH)/2,
                it.texW,
                it.texH
            };
            SDL_RenderCopy(renderer_, it.tex, nullptr, &dst);
        } else {
            // fallback: draw a small rect when no text available
            SDL_SetRenderDrawColor(renderer_, 120,120,120,200);
            SDL_Rect dot{ x_ + 8, itemRect.y + 6, 6, 6 };
            SDL_RenderFillRect(renderer_, &dot);
        }
    }
}