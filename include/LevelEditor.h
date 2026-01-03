#pragma once
#include "Level.h"

class LevelEditor {
public:
    LevelEditor(Level* l, int w, int h, float scale = 1.0f);
    void handleMouse(int mx, int my);

private:
    Level* level;
    int windowW;
    int windowH;
    float tileScale;
};