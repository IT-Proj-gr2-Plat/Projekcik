// cpp
#pragma once
#include <SDL.h>
#include <SDL_ttf.h>
#include <string>
#include <vector>
#include <functional>

class Menu {
public:
    Menu(SDL_Renderer* renderer, const char* fontPath, int fontSize);
    ~Menu();

    void addItem(const std::string &label, std::function<void()> cb);
    void handleEvent(const SDL_Event &e); // keyboard / mouse navigation
    void render();
    void toggle();
    bool visible() const;

private:
    struct Item {
        std::string label;
        std::function<void()> cb;
        SDL_Texture* tex = nullptr;
        int texW = 0, texH = 0;
    };
    std::vector<Item> items_;
    size_t selected_ = 0;
    bool visible_ = false;

    SDL_Renderer* renderer_ = nullptr;
    TTF_Font* font_ = nullptr;

    // layout
    int x_ = 60, y_ = 60, w_ = 380, item_h_ = 28, padding_ = 8;
    SDL_Color bg_{0,0,0,200}, sel_{30,144,255,220}, border_{200,200,200,200}, textCol_{240,240,240,255};

    void createLabelTexture(Item &it);
    void destroyTextures();
};