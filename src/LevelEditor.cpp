#include "LevelEditor.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

LevelEditor::LevelEditor(Level* l, int w, int h, float scale, int baseTile)
    : level(l), windowW(w), windowH(h), tileScale(scale), baseTilePixels(baseTile) {}

void LevelEditor::handleMouse(float mx, float my, float camX_editor_f){
    if (!level) return;
    if (windowW <= 0 || windowH <= 0) return;

    float cellWf = std::max(1.0f, baseTilePixels * tileScale);
    float cellHf = cellWf;

    float worldX_editor_f = mx + camX_editor_f;
    float worldY_editor_f = my;

    int col = static_cast<int>(std::floor(worldX_editor_f / cellWf));
    int row = static_cast<int>(std::floor(worldY_editor_f / cellHf));

    if (row < 0 || col < 0) return;

    // Ensure the grid is large enough and cycle the cell
    // 0 -> 1 -> 2 -> 3 -> 5 -> 0 (empty -> solid -> damaging -> pickup -> enemy spawn -> empty)
    level->ensureCell(row, col);
    int current = level->grid[row][col];
    if (current == 3 || current == 4 || current == 6 || current == 7) {
        level->grid[row][col] = 5;
    } else if (level->grid[row][col] == 5) {
        level->grid[row][col] = 8;
    } else if (level->grid[row][col] == 8) {
        level->grid[row][col] = 9;
    } else if (level->grid[row][col] == 9) {
        level->grid[row][col] = 10;
    }
    else if (level->grid[row][col] == 10) {
        level->grid[row][col] = 0;
    } else {
        level->grid[row][col] = (current + 1) % 6;
        if (level->grid[row][col] == 3) {
            int types[4] = {3,4,6,7};
            level->grid[row][col] = types[rand() % 4];
        }
    }
}