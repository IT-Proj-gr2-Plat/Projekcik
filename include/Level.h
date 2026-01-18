#pragma once

#include <SDL.h>
#include <string>
#include <vector>

class Level {
public:
    Level();
    ~Level();

    // Background update and render
    void updateBackground(float dt);
    void renderBackground(SDL_Renderer* renderer);

    // control background repeat
    void setBackgroundRepeat(bool repeat);

    // Setters
    void setBackgroundTexture(SDL_Texture* tex);
    void setFrameSize(int width, int height);
    void setScrollSpeed(float speed);

    // Parallax: 0 = fixed, 1 = follow
    void setParallax(float factor);

    // Update background offset based on camera X position
    void setBackgroundOffsetFromCamera(float camX, float maxCam, float dt);

    // Set maximum background speed in pixels/sec (<=0 = unlimited)
    void setBackgroundMaxSpeed(float pxPerSec);

    // Add getters for frame size
    int getFrameWidth() const;
    int getFrameHeight() const;

    // Level grid
    int rows;
    int cols;
    std::vector<std::vector<int>> grid;
    std::string backgroundPath;
    std::vector<std::string> usedAssets;
    std::vector<std::pair<int, int>> enemyPositions;

    // Editor helpers
    void toggleCell(int r, int c);
    void ensureCell(int r, int c);

    // Persist level
    bool saveToZip(const std::string& path, const std::string& assetsDir) const;
    bool loadFromFile(const std::string& path);

private:
    SDL_Texture* bgTexture;
    int frameWidth;
    int frameHeight;

    // Background scroll state
    float bgOffset;
    float scrollSpeed;

    // Background repeat flag
    bool bgRepeat;

    // Parallax factor
    float parallax;

    // Previous camera X for delta calculations
    float prevCamX;
    bool prevCamValid;

    // Maximum background movement speed in pixels/sec for camera-driven updates.
    float bgMaxSpeed;
};