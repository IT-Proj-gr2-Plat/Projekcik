#include "Level.h"
#include "ZipUtil.h"
#include <algorithm>
#include <sstream>

Level::Level(){
    grid.assign(rows * cols, 0);
}

void Level::resizeCols(int newCols){
    std::vector<int> g(rows * newCols, 0);
    for(int r = 0; r < rows; ++r){
        for(int c = 0; c < std::min(cols, newCols); ++c){
            g[r * newCols + c] = grid[r * cols + c];
        }
    }
    cols = newCols;
    grid.swap(g);
}

void Level::toggleCell(int c, int r){
    if(c < 0 || c >= cols || r < 0 || r >= rows) return;
    grid[r * cols + c] = !grid[r * cols + c];
}

bool Level::saveToZip(const std::string& zipPath){
    std::ostringstream layout;
    for(int r = 0; r < rows; ++r){
        for(int c = 0; c < cols; ++c) layout << (grid[r * cols + c] ? '1' : '0');
        if(r < rows - 1) layout << '\n';
    }

    std::ostringstream assets;
    assets << "background=" << backgroundPath << '\n';
    for(const auto &a : usedAssets) assets << a << '\n';

    return saveLevelZip(zipPath, layout.str(), assets.str());
}