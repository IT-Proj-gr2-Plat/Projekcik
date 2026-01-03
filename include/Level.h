#ifndef LEVEL_H
#define LEVEL_H

#include <SDL.h>
#include <string>
#include <vector>

class Level {
public:
    Level();
    ~Level();

    // Background update / render
    void updateBackground(float dt);
    void renderBackground(SDL_Renderer* renderer);

    // Setters
    void setBackgroundTexture(SDL_Texture* tex);
    void setFrameSize(int width, int height);
    void setScrollSpeed(float speed);

    // Editor / main access (kept public for direct use in existing code)
    int rows;                                        // number of grid rows
    int cols;                                        // number of grid cols
    std::vector<std::vector<int>> grid;              // grid cells (0/1 or tile ids)
    std::string backgroundPath;                      // path to background asset
    std::vector<std::string> usedAssets;             // list of used asset paths/names

    // Editor helpers
    void toggleCell(int r, int c);

    // Persist level (simple dump to file; not a real zip archive)
    bool saveToZip(const std::string& path) const;

private:
    SDL_Texture* bgTexture;
    int frameWidth;
    int frameHeight;

    // Background scroll state
    float bgOffset;
    float scrollSpeed;
};

#endif // LEVEL_H