#pragma once
#include <vector>
#include <string>

class Level {
public:
    int cols = 20;
    int rows = 9;
    std::vector<int> grid; // row-major
    std::string backgroundPath;
    std::vector<std::string> usedAssets;

    Level();
    void resizeCols(int newCols);
    void toggleCell(int c, int r);
    bool saveToZip(const std::string& zipPath); // uses minizip
};
