#include "LevelEditor.h"

LevelEditor::LevelEditor(Level* l, int w, int h): level(l), windowW(w), windowH(h){}

void LevelEditor::handleMouse(int mx, int my){
    if(!level) return;
    int cellH = windowH / level->rows;
    int c = mx / cellH;
    int r = my / cellH;
    level->toggleCell(c, r);
}