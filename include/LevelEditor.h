#pragma once
#include "Level.h"
#include <SDL.h>

class LevelEditor {
public:
    Level* level = nullptr;
    int windowW = 1280, windowH = 720;

    LevelEditor(Level* l, int w, int h);
    void handleMouse(int mx, int my);
};