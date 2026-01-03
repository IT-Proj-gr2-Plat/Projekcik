#include "LevelEditor.h"

LevelEditor::LevelEditor(Level* l, int w, int h, float scale)
    : level(l), windowW(w), windowH(h), tileScale(scale) {}

void LevelEditor::handleMouse(int mx, int my){
    if (!level) return;
    if (level->rows <= 0 || level->cols <= 0) return;
    if (windowW <= 0 || windowH <= 0) return;

    // compute separate cell width/height and apply tileScale
    int baseCellW = windowW / level->cols;
    int baseCellH = windowH / level->rows;
    if (baseCellW <= 0 || baseCellH <= 0) return;

    int cellW = std::max(1, (int)(baseCellW * tileScale));
    int cellH = std::max(1, (int)(baseCellH * tileScale));

    // map mouse -> column/row
    int col = mx / cellW;
    int row = my / cellH;

    // validate indices and bounds
    if (row < 0 || row >= level->rows) return;
    if (col < 0 || col >= level->cols) return;
    if (row >= (int)level->grid.size() || col >= (int)level->grid[row].size()) return;

    level->grid[row][col] = level->grid[row][col] ? 0 : 1;
}