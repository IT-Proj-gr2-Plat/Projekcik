#include "Level.h"
#include <SDL.h>
#include <cmath>
#include <fstream>
#include <sstream>

Level::Level()
    : bgTexture(nullptr)
    , frameWidth(800)
    , frameHeight(600)
    , bgOffset(0.0f)
    , scrollSpeed(100.0f)
    , rows(10)
    , cols(16)
{
    // initialize grid with zeros
    grid.assign(rows, std::vector<int>(cols, 0));
}

Level::~Level() = default;

void Level::setBackgroundTexture(SDL_Texture* tex) {
    bgTexture = tex;
}

void Level::setFrameSize(int width, int height) {
    frameWidth = width;
    frameHeight = height;
}

void Level::setScrollSpeed(float speed) {
    scrollSpeed = speed;
}

void Level::updateBackground(float dt) {
    if (!bgTexture) return;

    int texW = 0, texH = 0;
    SDL_QueryTexture(bgTexture, nullptr, nullptr, &texW, &texH);
    if (texH == 0) return;

    // Scale so background height matches frame height
    float scale = static_cast<float>(frameHeight) / static_cast<float>(texH);
    int scaledW = static_cast<int>(texW * scale);
    if (scaledW <= 0) return;

    // Advance offset in pixels (on the scaled image)
    bgOffset += scrollSpeed * dt;

    // Keep offset within [0, scaledW)
    bgOffset = std::fmod(bgOffset, static_cast<float>(scaledW));
    if (bgOffset < 0.0f) bgOffset += static_cast<float>(scaledW);
}

void Level::renderBackground(SDL_Renderer* renderer) {
    if (!bgTexture || !renderer) return;

    int texW = 0, texH = 0;
    SDL_QueryTexture(bgTexture, nullptr, nullptr, &texW, &texH);
    if (texH == 0) return;

    float scale = static_cast<float>(frameHeight) / static_cast<float>(texH);
    int scaledW = static_cast<int>(texW * scale);
    int scaledH = frameHeight;
    if (scaledW <= 0) return;

    // Start drawing so that the first copy begins at -offset
    int startX = -static_cast<int>(bgOffset);

    // Draw copies until screen is filled (usually 2 copies suffice)
    for (int x = startX; x < frameWidth; x += scaledW) {
        SDL_Rect dst{ x, 0, scaledW, scaledH };
        SDL_RenderCopy(renderer, bgTexture, nullptr, &dst);
    }
}

void Level::toggleCell(int r, int c) {
    if (r < 0 || c < 0) return;
    // ensure grid has enough rows/cols
    if (r >= rows) {
        grid.resize(r + 1);
        for (int i = rows; i <= r; ++i) grid[i].assign(cols, 0);
        rows = r + 1;
    }
    if (c >= cols) {
        for (auto &row : grid) row.resize(c + 1, 0);
        cols = c + 1;
    }
    // toggle between 0 and 1
    grid[r][c] = (grid[r][c] == 0) ? 1 : 0;
}

bool Level::saveToZip(const std::string& path) const {
    // For build/time reasons this writes a plain JSON-like dump to the given path.
    // Replace with a real .zip writer (minizip, libzip, etc.) when desired.
    std::ofstream ofs(path, std::ios::binary);
    if (!ofs) return false;

    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"rows\": " << rows << ",\n";
    ss << "  \"cols\": " << cols << ",\n";
    ss << "  \"backgroundPath\": \"" << backgroundPath << "\",\n";
    ss << "  \"usedAssets\": [";
    for (size_t i = 0; i < usedAssets.size(); ++i) {
        if (i) ss << ", ";
        ss << "\"" << usedAssets[i] << "\"";
    }
    ss << "],\n";
    ss << "  \"grid\": [\n";
    for (int r = 0; r < rows; ++r) {
        ss << "    [";
        for (int c = 0; c < cols; ++c) {
            if (c) ss << ", ";
            int v = 0;
            if (r < static_cast<int>(grid.size()) && c < static_cast<int>(grid[r].size())) v = grid[r][c];
            ss << v;
        }
        ss << "]";
        if (r < rows - 1) ss << ",";
        ss << "\n";
    }
    ss << "  ]\n";
    ss << "}\n";

    std::string data = ss.str();
    ofs.write(data.data(), static_cast<std::streamsize>(data.size()));
    return ofs.good();
}