#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
#include <iostream>
#include "Texture.h"
#include "Player.h"
#include "Level.h"
#include "LevelEditor.h"
#include "Menu.h"
#include "MainMenu.h"
#include "Enemy.h"
#include "Boss.h"
#include "GameObjects.h"
#include "Collision.h"
#include "SaveData.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
#include <cstdint>
#include <cstdlib>

Mix_Chunk* globalPickSound = nullptr;
Mix_Chunk* globalStepSound = nullptr;
Mix_Chunk* globalDeadSound = nullptr;




int main(int argc, char* argv[]) {
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0){
        std::cerr << "SDL_Init Error: " << SDL_GetError() << "\n";
        return 1;
    }

    int imgFlags = IMG_INIT_JPG | IMG_INIT_PNG;
    if((IMG_Init(imgFlags) & imgFlags) != imgFlags){
        std::cerr << "IMG_Init failed: " << IMG_GetError() << "\n";
    }

    if(TTF_Init() != 0){
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "TTF_Init failed: %s", TTF_GetError());
    }

    // Initialize SDL_mixer
    if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0){
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError());
        // Perhaps exit if audio is critical
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Audio Error", "Failed to initialize audio. The game may not have sound.", nullptr);
    }

    // Scaling fix
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

    const int WINH = 288, WINW = 512;
    SDL_Window* win = SDL_CreateWindow("Projekcik", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINW, WINH, SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_SHOWN);
    if(!win){ std::cerr << "CreateWindow failed\n"; IMG_Quit(); SDL_Quit(); return 1; }
    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if(!ren){ std::cerr << "CreateRenderer failed\n"; SDL_DestroyWindow(win); IMG_Quit(); SDL_Quit(); return 1; }

    // keep logical game coords at WINW x WINH even in fullscreen
    SDL_RenderSetLogicalSize(ren, WINW, WINH);

    // asset path setup
    char* basePath = SDL_GetBasePath();
    std::string assetsDir;
    if (basePath) {
        assetsDir = std::string(basePath) + "assets/";
        SDL_free(basePath);
    } else {
        assetsDir = "assets/";
    }

    // Load audio
    Mix_Music* menuMusic = Mix_LoadMUS((assetsDir + "menu_muzyka.mp3").c_str());
    Mix_Music* levelMusic = Mix_LoadMUS((assetsDir + "muzyczkaa_poziomy.mp3").c_str());
    Mix_Music* bossMusic = Mix_LoadMUS((assetsDir + "boss_muzyka.mp3").c_str());
    Mix_Chunk* deadSound = Mix_LoadWAV((assetsDir + "deadzik.mp3").c_str());
    Mix_Chunk* pickSound = Mix_LoadWAV((assetsDir + "pick_up.mp3").c_str());
    Mix_Chunk* stepSound = Mix_LoadWAV((assetsDir + "step.mp3").c_str());
    
    if (!menuMusic) SDL_Log("Failed to load menu music: %s", Mix_GetError());
    if (!levelMusic) SDL_Log("Failed to load level music: %s", Mix_GetError());
    if (!bossMusic) SDL_Log("Failed to load boss music: %s", Mix_GetError());
    if (!deadSound) SDL_Log("Failed to load dead sound: %s", Mix_GetError());
    if (!pickSound) SDL_Log("Failed to load pick sound: %s", Mix_GetError());
    if (!stepSound) SDL_Log("Failed to load step sound: %s", Mix_GetError());

    // Set global pointers
    globalPickSound = pickSound;
    globalStepSound = stepSound;
    globalDeadSound = deadSound;

    // Set volumes
    Mix_VolumeMusic(64); // max volume for music
    Mix_Volume(-1, 128);  // max volume for chunks

    TTF_Font* hudFont = nullptr;
    std::string hudFontPath = (assetsDir + "BreeSerif-Regular.otf");
    hudFont = TTF_OpenFont(hudFontPath.c_str(), 24); // larger for game over screens
    if(!hudFont){
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "HUD font not opened: %s", TTF_GetError());
        hudFont = nullptr;
    }

    // Load save data
    SaveData saveData = loadProgress();

    // Main game loop
    while (true) {
        // Play menu music
        if (menuMusic) Mix_PlayMusic(menuMusic, -1);

        // Calculate max level
        int maxLevel = 1;
        for(int i=1; i<32; i++) if(saveData.completedLevels & (1<<i)) maxLevel = std::min(9, i + 1);
        if (saveData.hasKey) maxLevel = 10;

        // Show main menu
        MainMenu mainMenu(ren, assetsDir, maxLevel);
        int selectedLevel = mainMenu.run();
        if (selectedLevel == -1) break; // kill

        

        std::string bgFile = "poziom_" + std::to_string(selectedLevel) + "_tlo.jpg";
        if (selectedLevel == 10) bgFile = "boss_tlo.png";

        // Load assets using assetsDir
        Texture bgTex;
        bgTex.load(ren, (assetsDir + bgFile).c_str());

        Texture f1,f2,f3,f4,f5,f6;
        f1.load(ren, (assetsDir + "chodzenie_1.png").c_str());
        f2.load(ren, (assetsDir + "chodzenie_2.png").c_str());
        f3.load(ren, (assetsDir + "chodzenie_3.png").c_str());
        f4.load(ren, (assetsDir + "ochroniarz_1.png").c_str());
        f5.load(ren, (assetsDir + "ochroniarz_2.png").c_str());
        f6.load(ren, (assetsDir + "ochroniarz_3.png").c_str());

        // Load pickup textures
        Texture piwo1, piwo2, piwoKufel, pollitroka, piwoButelka, woda, pollitrowka3, pollitrowka2, zelazo;
        piwo1.load(ren, (assetsDir + "piwo_1.png").c_str());
        piwo2.load(ren, (assetsDir + "piwo_2.png").c_str());
        piwoKufel.load(ren, (assetsDir + "piwo_w_kuflu.png").c_str());
        pollitroka.load(ren, (assetsDir + "pollitroka_1.png").c_str());
        piwoButelka.load(ren, (assetsDir + "piwo_w_butelce.png").c_str());
        woda.load(ren, (assetsDir + "woda.png").c_str());
        pollitrowka3.load(ren, (assetsDir + "pollitrowka_3.png").c_str());
        pollitrowka2.load(ren, (assetsDir + "pollitrowka_2.png").c_str());
        zelazo.load(ren, (assetsDir + "zelazo.png").c_str());

        // Load boss textures
        Texture boss1, boss2, boss3;
        boss1.load(ren, (assetsDir + "boss_1.png").c_str());
        boss2.load(ren, (assetsDir + "boss_2.png").c_str());
        boss3.load(ren, (assetsDir + "boss_3.png").c_str());

        // Abort gracefully if required textures are missing
        if (!bgTex.tex || !f1.tex || !f2.tex || !f3.tex) {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Missing assets",
                                     "One or more assets failed to load. Ensure the `assets` folder is next to the executable or adjust the working directory.",
                                     win);
            continue; // back to menu
        }

        // Play level music
        if (selectedLevel == 10) {
            if (bossMusic) Mix_PlayMusic(bossMusic, -1);
        } else {
            if (levelMusic) Mix_PlayMusic(levelMusic, -1);
            }

        // Use logical WINW/WINH for level/frame sizing and rendering math
        Level level;
        level.setFrameSize(WINW, WINH);

        // Ensure rows is initialized before allocating the grid (tile size = 32)
        level.rows = WINH / 32 + 1;

        level.cols = 156; // map size
        if (selectedLevel == 10) level.cols = 16;
        level.grid.assign(level.rows, std::vector<int>(level.cols, 0));
        SDL_Log("DBG: level frame size set to %dx%d", WINW, WINH);
        int groundRow = level.rows - 2;
        if (groundRow >= 0) {
            for (int c = 0; c < level.cols; ++c) {
                level.grid[groundRow][c] = 1; // solid ground
            }
        }

        level.setBackgroundTexture(bgTex.tex);
        level.setBackgroundRepeat(false); // scroll once
        level.setScrollSpeed(0.0f); // no auto-scroll
        level.setParallax(0.25f); // parallax
        level.setBackgroundMaxSpeed(50.0f); // max 50 px/sec

        Player player;
        player.frames = { &f3, &f2, &f3, &f1 };
        player.width = 32; player.height = 48;
        player.x = 10.f;

        // Place player on ground initially
        int levelH = level.rows * 32; // base tile = 32
        player.y = static_cast<float>(std::max<int>(0, levelH - player.height)); // put player on bottom of level
        player.onGround = true;
        player.vy = 0.0f;

        level.backgroundPath = bgFile;
        level.usedAssets = { assetsDir + "chodzenie_1.png", assetsDir + "chodzenie_2.png", assetsDir + "chodzenie_3.png", assetsDir + "ochroniarz_1.png", assetsDir + "ochroniarz_2.png", assetsDir + "ochroniarz_3.png", assetsDir + "piwo_1.png", assetsDir + "piwo_2.png", assetsDir + "piwo_w_kuflu.png", assetsDir + "pollitroka_1.png", assetsDir + "piwo_w_butelce.png", assetsDir + "woda.png", assetsDir + "pollitrowka_3.png", assetsDir + "pollitrowka_2.png", assetsDir + "zelazo.png" };

        // Declare game variables
        std::vector<Enemy> enemies;
        std::vector<Projectile> projectiles;
        std::vector<Blood> bloods;
        Boss boss;
        bool playerLost = false;
        bool playerWon = false;

        if (selectedLevel == 10) {
            boss.frames = {&boss1, &boss2, &boss3};
            boss.x = 8 * 32.0f;
            boss.y = (level.rows - 2) * 32.0f;
        }

        const float editorTileScale = 1.0f;   // used only by LevelEditor
        const float renderTileScale = 1.0f;   // used for runtime drawing / player size scaling
        const int baseTilePixels = 32;        // physical base tile size (used for collision/camera)
        float camX = 0.0f;
        float editorCamX = 0.0f;

        // Try to load level from file
        std::string levelFile = "level_" + std::to_string(selectedLevel) + ".zip";
        LevelEditor* editor = nullptr;
        bool loaded = level.loadFromFile(levelFile);
        if (loaded) {
            // Reload background texture
            bgTex.load(ren, (assetsDir + level.backgroundPath).c_str());
            level.setBackgroundTexture(bgTex.tex);
            level.setBackgroundRepeat(false);
            level.setScrollSpeed(0.0f);
            level.setParallax(0.25f);
            level.setBackgroundMaxSpeed(50.0f);
            // Recreate editor with new level size
            if (editor) delete editor;
            editor = new LevelEditor(&level, WINW, WINH, editorTileScale, baseTilePixels);
            // Reset player position
            player.x = 10.f;
            int levelH = level.rows * 32;
            player.y = static_cast<float>(std::max(0, levelH - player.height));
            player.onGround = true;
            player.vy = 0.0f;
            // Reset camera
            camX = 0.0f;
            editorCamX = 0.0f;
            // Repopulate enemies from the loaded grid
            enemies.clear();
            for (auto& p : level.enemyPositions) {
                if (p.first >= 0 && p.first < level.rows && p.second >= 0 && p.second < level.cols) {
                    level.grid[p.first][p.second] = 5;
                }
            }
            for (int r = 0; r < level.rows; ++r) {
                for (int c = 0; c < level.cols; ++c) {
                    if (level.grid[r][c] == 5) {
                        Enemy e;
                        e.frames = { &f4, &f5, &f6 };
                        e.width = 32; e.height = 48;
                        e.x = (float)c * 32.0f;
                        e.y = ((float)r + 1) * 32.0f;
                        e.vx = 50.f;
                        e.vy = 0.f;
                        e.onGround = true;
                        e.active = true;
                        enemies.push_back(e);
                    }
                }
            }
        }

        if (!loaded) {
            for (int r = 0; r < level.rows; ++r) {
                for (int c = 0; c < level.cols; ++c) {
                    if (level.grid[r][c] == 5) {
                        Enemy e;
                        e.frames = { &f4, &f5, &f6 };
                        e.width = 32; e.height = 48;
                        e.x = (float)c * 32.0f;
                        e.y = ((float)r + 1) * 32.0f; // on top of tile
                        e.vx = 50.f;
                        e.vy = 0.f;
                        e.onGround = true;
                        e.active = true;
                        enemies.push_back(e);
                    }
                }
            }
            // create editor
            editor = new LevelEditor(&level, WINW, WINH, editorTileScale, baseTilePixels);
        }


        // Menu setup
        Menu menu(ren, (assetsDir + "BreeSerif-Regular.otf").c_str(), 18);
        menu.addItem("Reload textures", [&](){
            f1.load(ren, (assetsDir + "chodzenie_1.png").c_str());
            f2.load(ren, (assetsDir + "chodzenie_2.png").c_str());
            f3.load(ren, (assetsDir + "chodzenie_3.png").c_str());
            f4.load(ren, (assetsDir + "ochroniarz_1.png").c_str());
            f5.load(ren, (assetsDir + "ochroniarz_2.png").c_str());
            f6.load(ren, (assetsDir + "ochroniarz_3.png").c_str());
            piwo1.load(ren, (assetsDir + "piwo_1.png").c_str());
            piwo2.load(ren, (assetsDir + "piwo_2.png").c_str());
            piwoKufel.load(ren, (assetsDir + "piwo_w_kuflu.png").c_str());
            pollitroka.load(ren, (assetsDir + "pollitroka_1.png").c_str());
            piwoButelka.load(ren, (assetsDir + "piwo_w_butelce.png").c_str());
            woda.load(ren, (assetsDir + "woda.png").c_str());
            pollitrowka3.load(ren, (assetsDir + "pollitrowka_3.png").c_str());
            pollitrowka2.load(ren, (assetsDir + "pollitrowka_2.png").c_str());
            zelazo.load(ren, (assetsDir + "zelazo.png").c_str());
            boss1.load(ren, (assetsDir + "boss_1.png").c_str());
            boss2.load(ren, (assetsDir + "boss_2.png").c_str());
            boss3.load(ren, (assetsDir + "boss_3.png").c_str());

            // Reload background texture
            bgTex.load(ren, (assetsDir + level.backgroundPath).c_str());
            level.setBackgroundTexture(bgTex.tex);
            level.setBackgroundRepeat(false);
            level.setScrollSpeed(0.0f);
            level.setParallax(0.25f);
            level.setBackgroundMaxSpeed(50.0f);
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Menu", "Textures reloaded", win);
    });
        menu.addItem("Save level", [&](){
            level.enemyPositions.clear();
            for (int r = 0; r < level.rows; ++r) {
                for (int c = 0; c < level.cols; ++c) {
                    if (level.grid[r][c] == 5) {
                        level.enemyPositions.push_back({r, c});
                    }
                }
            }
            level.saveToZip("level_" + std::to_string(selectedLevel) + ".zip", assetsDir);
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Menu", "Level saved", win);
        });

        menu.addItem("Load level", [&](){
            // Backup current level state
            int oldRows = level.rows;
            int oldCols = level.cols;
            std::vector<std::vector<int>> oldGrid = level.grid;
            std::vector<std::pair<int, int>> oldEnemyPositions = level.enemyPositions;
            std::string oldBackgroundPath = level.backgroundPath;

            // Try to load level from file
            std::string levelFile = "level_" + std::to_string(selectedLevel) + ".zip";
            bool loaded = level.loadFromFile(levelFile);
            if (loaded) {
                // Reload background texture
                bgTex.load(ren, (assetsDir + level.backgroundPath).c_str());
                level.setBackgroundTexture(bgTex.tex);
                level.setBackgroundRepeat(false);
                level.setScrollSpeed(0.0f);
                level.setParallax(0.25f);
                level.setBackgroundMaxSpeed(50.0f);
                // Recreate editor with new level size
                delete editor;
                editor = new LevelEditor(&level, WINW, WINH, editorTileScale, baseTilePixels);
                // Reset player position
                player.x = 10.f;
                int levelH = level.rows * 32;
                player.y = static_cast<float>(std::max(0, levelH - player.height));
                player.onGround = true;
                player.vy = 0.0f;
                // Reset camera
                camX = 0.0f;
                editorCamX = 0.0f;
                // Repopulate enemies from the loaded grid
                enemies.clear();
                for (auto& p : level.enemyPositions) {
                    if (p.first >= 0 && p.first < level.rows && p.second >= 0 && p.second < level.cols) {
                        level.grid[p.first][p.second] = 5;
                    }
                }
                for (int r = 0; r < level.rows; ++r) {
                    for (int c = 0; c < level.cols; ++c) {
                        if (level.grid[r][c] == 5) {
                            Enemy e;
                            e.frames = { &f4, &f5, &f6 };
                            e.width = 32; e.height = 48;
                            e.x = (float)c * 32.0f;
                            e.y = ((float)r + 1) * 32.0f;
                            e.vx = 50.f;
                            e.vy = 0.f;
                            e.onGround = true;
                            e.active = true;
                            enemies.push_back(e);
                        }
                    }
                }
                SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Menu", "Level loaded", win);
            } else {
                // Restore level state on failure
                level.rows = oldRows;
                level.cols = oldCols;
                level.grid = oldGrid;
                level.enemyPositions = oldEnemyPositions;
                level.backgroundPath = oldBackgroundPath;
                SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Menu", "Failed to load level", win);
            }
        });

        menu.addItem("Reload level", [&](){
            if (level.loadFromFile("level_saved.zip")) {
                // Reload background texture
                bgTex.load(ren, (assetsDir + level.backgroundPath).c_str());
                level.setBackgroundTexture(bgTex.tex);
                level.setBackgroundRepeat(false);
                level.setScrollSpeed(0.0f);
                level.setParallax(0.25f);
                level.setBackgroundMaxSpeed(50.0f);
                // Recreate editor with new level size
                delete editor;
                editor = new LevelEditor(&level, WINW, WINH, editorTileScale, baseTilePixels);
                // Reset player position
                player.x = 10.f;
                int levelH = level.rows * 32;
                player.y = static_cast<float>(std::max(0, levelH - player.height));
                player.onGround = true;
                player.vy = 0.0f;
                // Reset camera
                camX = 0.0f;
                editorCamX = 0.0f;
                // Repopulate enemies from loaded grid
                enemies.clear();
                for (auto& p : level.enemyPositions) {
                    if (p.first >= 0 && p.first < level.rows && p.second >= 0 && p.second < level.cols) {
                        level.grid[p.first][p.second] = 5;
                    }
                }
                for (int r = 0; r < level.rows; ++r) {
                    for (int c = 0; c < level.cols; ++c) {
                        if (level.grid[r][c] == 5) {
                            Enemy e;
                            e.frames = { &f4, &f5, &f6 };
                            e.width = 32; e.height = 48;
                            e.x = (float)c * 32.0f;
                            e.y = ((float)r + 1) * 32.0f;
                            e.vx = 50.f;
                            e.vy = 0.f;
                            e.onGround = true;
                            e.active = true;
                            enemies.push_back(e);
                            // level.grid[r][c] = 0;
                        }
                    }
                }
                SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Menu", "Level reloaded", win);
            } else {
                SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Menu", "Failed to reload level", win);
            }
        });

        bool running = true;
        bool editMode = false;
        float fade = 0.0f;
        Uint64 last = SDL_GetPerformanceCounter();

        // Game loop
        while(running) {
            const Uint8* kb = SDL_GetKeyboardState(nullptr);
            static int pickupFrame = 0;
            static float stepCooldown = 0.0f;
            static double fpsTimer = 0.0;
            static std::string currentFpsText = "FPS: 60";
            pickupFrame++;

            Uint64 now = SDL_GetPerformanceCounter();
            double dt = (double)(now - last) / (double)SDL_GetPerformanceFrequency();
            last = now;

            stepCooldown -= dt;
            fpsTimer += dt;

            // Update current FPS text every 0.5 seconds
            if (fpsTimer >= 0.5) {
                int currentFps = static_cast<int>(1.0 / dt);
                currentFpsText = "FPS: " + std::to_string(currentFps);
                fpsTimer = 0.0;
            }

            SDL_Event ev;
            while (SDL_PollEvent(&ev)) {
                if (ev.type == SDL_QUIT) { running = false; break; }

                if (ev.type == SDL_WINDOWEVENT && ev.window.event == SDL_WINDOWEVENT_RESIZED) {
                    // Recreate editor on window size change
                    delete editor;
                    editor = new LevelEditor(&level, WINW, WINH, editorTileScale, baseTilePixels);
                    continue;
                }

                if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_m) {
                    menu.toggle();
                    continue;
                }

                if (menu.visible()) {
                    menu.handleEvent(ev);
                    continue;
                }

                if (ev.type == SDL_KEYDOWN) {
                    if (ev.key.keysym.scancode == SDL_SCANCODE_E) {
                        editMode = !editMode;
                        if (editMode) {
                            editorCamX = camX;
                            // Show enemy spawns in grid
                            for (auto& p : level.enemyPositions) {
                                if (p.first >= 0 && p.first < level.rows && p.second >= 0 && p.second < level.cols) {
                                    level.grid[p.first][p.second] = 5;
                                }
                            }
                        } else {
                            // Update enemyPositions from grid
                            level.enemyPositions.clear();
                            for (int r = 0; r < level.rows; ++r) {
                                for (int c = 0; c < level.cols; ++c) {
                                    if (level.grid[r][c] == 5) {
                                        level.enemyPositions.push_back({r, c});
                                    }
                                }
                            }
                            // Repopulate enemies from grid
                            enemies.clear();
                            for (auto& p : level.enemyPositions) {
                                Enemy e;
                                e.frames = { &f4, &f5, &f6 };
                                e.width = 32; e.height = 48;
                                e.x = p.second * 32.0f;
                                e.y = (p.first + 1) * 32.0f;
                                e.vx = 50.f;
                                e.vy = 0.f;
                                e.onGround = true;
                                enemies.push_back(e);
                                level.grid[p.first][p.second] = 5; // remember
                            }
                        }
                        continue;
                    }
                    if (ev.key.keysym.scancode == SDL_SCANCODE_S && (SDL_GetModState() & KMOD_CTRL)) {
                        level.saveToZip("level_saved.zip", assetsDir);
                        continue;
                    }
                    if (ev.key.keysym.scancode == SDL_SCANCODE_F11) {
                        Uint32 flags = SDL_GetWindowFlags(win);
                        if (flags & SDL_WINDOW_FULLSCREEN_DESKTOP) {
                            SDL_SetWindowFullscreen(win, 0);
                        } else {
                            SDL_SetWindowFullscreen(win, SDL_WINDOW_FULLSCREEN_DESKTOP);
                        }
                        continue;
                    }
                }

                if (editMode && ev.type == SDL_KEYDOWN) {
                    if (ev.key.keysym.scancode == SDL_SCANCODE_LEFT) editorCamX -= 32.0f;
                    if (ev.key.keysym.scancode == SDL_SCANCODE_RIGHT) editorCamX += 32.0f;
                    float maxCam = std::max(0.0f, (float)(level.cols * baseTilePixels) - (float)WINW / renderTileScale);
                    editorCamX = std::max(0.0f, std::min(editorCamX, maxCam));
                    continue;
                }

                if (editMode && ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_LEFT) {
                    int winMouseX_evt = ev.button.x;
                    int winMouseY_evt = ev.button.y;
                    int winMouseX_state = 0, winMouseY_state = 0;
                    SDL_GetMouseState(&winMouseX_state, &winMouseY_state);

                    // Window size in window coordinates
                    int winW_win = 0, winH_win = 0;
                    SDL_GetWindowSize(win, &winW_win, &winH_win);
                    if (winW_win <= 0 || winH_win <= 0) continue;

                    // Renderer output size in pixels
                    int outW_pixels = 0, outH_pixels = 0;
                    SDL_GetRendererOutputSize(ren, &outW_pixels, &outH_pixels);
                    if (outW_pixels <= 0 || outH_pixels <= 0) continue;

                    // Logical size
                    int logicalW = 0, logicalH = 0;
                    SDL_RenderGetLogicalSize(ren, &logicalW, &logicalH);
                    if (logicalW <= 0 || logicalH <= 0) continue;

                    float lx = static_cast<float>(winMouseX_state) * static_cast<float>(logicalW) / static_cast<float>(winW_win);
                    float ly = static_cast<float>(winMouseY_state) * static_cast<float>(logicalH) / static_cast<float>(winH_win);

                    float editorScale = 1.0f / editorTileScale;
                    float mx_editor = lx * editorScale;
                    float my_editor = ly * editorScale;

                    // Compute floating camera offset in editor pixel-space (avoid rounding)
                    float camX_editor_f = camX * editorScale;

                    // Pass float logical coordinates to editor for precise mapping
                    editor->handleMouse(mx_editor, my_editor, camX_editor_f);
                    continue;
                }
                } // frame update & render
                int levelW = level.cols * baseTilePixels;
                int physCellW = baseTilePixels;
                int physCellH = baseTilePixels;
                if (!editMode && !playerLost && !playerWon) player.update(dt, kb);
                if (!editMode && player.onGround && fabs(player.vx) > 0.1f && stepCooldown <= 0.0f) {
                    if (globalStepSound) Mix_PlayChannel(-1, globalStepSound, 0);
                    stepCooldown = 0.75f;
                }
                if ((kb[SDL_SCANCODE_DOWN] || kb[SDL_SCANCODE_S] ) && player.projectileCooldown <= 0 && !editMode && !playerLost && !playerWon) {
                    Projectile p;
                    p.tex = &piwo1;
                    p.width = 16; p.height = 16;
                    p.x = player.x + player.width / 2.0f - p.width / 2.0f;
                    p.y = player.y - player.height / 2.0f - p.height / 2.0f;
                    p.vx = player.facingLeft ? -250 : 250;
                    p.vy = -400;
                    p.active = true;
                    p.fromPlayer = true;
                    projectiles.push_back(p);
                    player.projectileCooldown = 0.5f;
                }
                if (!editMode && !playerLost && !playerWon) {
                    for (auto& e : enemies) {
                        e.update(dt, levelW);
                    }
                    for (auto& e : enemies) {
                        Collision::resolveEnemyCollisions(e, level, physCellW, physCellH);
                    }
                }

                if (!editMode && !playerLost && !playerWon && selectedLevel == 10) {
                    boss.update((float)dt, player, projectiles, zelazo, levelW);
                    Collision::resolveBossCollisions(boss, level, physCellW, physCellH);
                }

                if (editMode) {
                    if (kb[SDL_SCANCODE_LEFT]) editorCamX -= 2000.0f * dt;
                    if (kb[SDL_SCANCODE_RIGHT]) editorCamX += 2000.0f * dt;
                    float maxCam = std::max(0.0f, (float)(level.cols * baseTilePixels) - (float)WINW / renderTileScale);
                    editorCamX = std::max(0.0f, std::min(editorCamX, maxCam));
                }

                // Update projectiles
                int levelH_now = level.rows * physCellH;
                for (auto& p : projectiles) {
                    if (p.hasPhysics) {
                        p.vy += 1200.f * dt;
                    } else {
                        p.lifetime -= dt;
                        if (p.lifetime <= 0) p.active = false;
                    }
                    p.x += p.vx * dt;
                    p.y += p.vy * dt;
                    if (p.x < -100 || p.x > levelW + 100 || p.y < -100 || p.y > levelH_now + 100) p.active = false;
                    if (p.active && p.hasPhysics) {
                        int col = (int)(p.x / physCellW);
                        int row = (int)(p.y / physCellH);
                        if (row >= 0 && row < level.rows && col >= 0 && col < level.cols && level.grid[row][col] == 1) {
                            p.active = false;
                        }
                    }
                }
                // Check projectile collision with player
                for (auto& p : projectiles) {
                    if (p.active) {
                        if (p.fromPlayer) {
                            // check with enemies
                            for (auto& e : enemies) {
                                if (p.x < e.x + e.width && p.x + p.width > e.x && p.y < e.y && p.y + p.height > e.y - e.height) {
                                    e.isDead = true;
                                    e.curFrame = 0;
                                    e.frameTime = 0;
                                    // Add blood particles
                                    for (int i = 0; i < 5; ++i) {
                                        Blood b;
                                        b.x = e.x + (rand() % e.width);
                                        b.y = e.y - e.height / 2.0f;
                                        b.vx = (rand() % 200) - 100;
                                        b.vy = -(rand() % 200);
                                        b.lifetime = 60;
                                        bloods.push_back(b);
                                    }
                                    p.active = false;
                                    break;
                                }
                            }
                            if (p.active && selectedLevel == 10) {
                                if (p.x < boss.x + boss.width && p.x + p.width > boss.x && p.y < boss.y && p.y + p.height > boss.y - boss.height) {
                                    boss.hp -= 1;
                                    p.active = false;
                                }
                            }
                        } else {
                            // check with player
                            if (p.x < player.x + player.width && p.x + p.width > player.x && p.y < player.y && p.y + p.height > player.y - player.height) {
                                if (player.invulnTimer <= 0.0f) {
                                    player.health -= 1;
                                    player.invulnTimer = player.invuln;
                                    if (player.health < 0) player.health = 0;
                                }
                                p.active = false;
                            }
                        }
                    }
                }
                // Remove inactive projectiles
                projectiles.erase(std::remove_if(projectiles.begin(), projectiles.end(), [](const Projectile& p){ return !p.active; }), projectiles.end());
                // Remove inactive enemies
                enemies.erase(std::remove_if(enemies.begin(), enemies.end(), [](const Enemy& e){ return !e.active; }), enemies.end());

                // Update bloods
                for (auto& b : bloods) {
                    b.vy += 1200.f * dt;
                    b.x += b.vx * dt;
                    b.y += b.vy * dt;
                    b.lifetime--;
                }
                bloods.erase(std::remove_if(bloods.begin(), bloods.end(), [](const Blood& b){ return b.lifetime <= 0; }), bloods.end());

                // Use logical size for drawing / camera math
                int winW = WINW;
                int winH = WINH;

                int renderCellW = std::max(1, (int)(baseTilePixels * renderTileScale + 0.5f));
                int renderCellH = renderCellW;

                float renderScale = (float)renderCellW / (float)physCellW;

                int worldW = std::max(levelW, winW);
                int worldH = std::max(levelH_now, winH);

                if (!editMode && !playerLost && !playerWon) {
                    Collision::resolvePlayerCollisions(player, level, physCellW , physCellH, saveData);

                    // Check collision with enemy
                    for (auto& enemy : enemies) {
                        float ex = enemy.x;
                        float ew = enemy.width;
                        float et = enemy.y;
                        float eh = enemy.height;
                        float px = player.x;
                        float pw = player.width;
                        float pt = player.y - player.height;
                        float ph = player.height;
                        if (px < ex + ew && px + pw > ex && pt < et && pt + ph > et - eh) {
                            if (player.invulnTimer <= 0.0f) {
                                player.health -= 1;
                                player.invulnTimer = player.invuln;
                                if (player.health < 0) player.health = 0;
                            }
                        }
                    }

                    // Check collision with boss
                    if (selectedLevel == 10) {
                        float bx = boss.x;
                        float bw = boss.width;
                        float bt = boss.y - boss.height;
                        float bh = boss.height;
                        float px = player.x;
                        float pw = player.width;
                        float pt = player.y - player.height;
                        float ph = player.height;
                        if (px < bx + bw && px + pw > bx && pt < bt + bh && pt + ph > bt) {
                            if (boss.invulnTimer <= 0.0f) {
                                boss.hp -= 1;
                                boss.invulnTimer = boss.invuln;
                            }
                        }
                    }

                    // Check for game over conditions
                    if (player.health <= 0) {
                        playerLost = true;
                        fade = 0.0f;
                        running = false;
                        if (globalDeadSound) Mix_PlayChannel(-1, globalDeadSound, 0);
                    }
                    if (player.x >= levelW - player.width && selectedLevel != 10) {
                        playerWon = true;
                        running = false;
                    }
                    if (selectedLevel == 10 && boss.hp <= 0) {
                        playerWon = true;
                        running = false;
                    }
                }

                // clamp player to level bounds (physics units)
                if (levelW > 0) {
                    if (player.x < 0.f) player.x = 0.f;
                    float maxPlayerX = (float)std::max(0, levelW - player.width);
                    if (player.x > maxPlayerX) player.x = maxPlayerX;
                }
                if (levelH_now > 0) {
                    if (player.y > levelH_now - player.height) {
                        player.health = 0;
                    } else {
                        if (player.y < 0.f) player.y = 0.f;
                        float maxPlayerY = (float)std::max(0, levelH_now - player.height);
                        if (player.y > maxPlayerY) { player.y = maxPlayerY; player.onGround = true; player.vy = 0.f; }
                    }
                }

                // Camera: center on player in physics units, clamp to level bounds
                float camTarget = player.x - ( (float)winW / (2.0f * renderScale) );
                float maxCam = std::max(0.0f, (float)(levelW) - (float)winW / renderScale);
                camX = std::max(0.0f, std::min(camTarget, maxCam));

                float levelWorldW = static_cast<float>(level.cols * physCellW);
                float camWidthWorld = static_cast<float>(winW) / renderScale;
                float maxCamWorld = std::max(0.0f, levelWorldW - camWidthWorld);

                float playerCenter = player.x + (player.width * 0.5f);
                float camTargetWorld = playerCenter - (camWidthWorld * 0.5f);
                float camX_world = std::max(0.0f, std::min(maxCamWorld, camTargetWorld));

                // keep camX in sync
                camX = camX_world;

                if (editMode) camX = editorCamX;

                // Compute floating render-space camera for background rendering
                float camX_render_f = camX * renderScale;
                float camMax_render_f = maxCamWorld * renderScale;

                // Integer camera for rendering tiles/player
                int camX_render = static_cast<int>(std::lround(camX_render_f));
                int camMax_render = static_cast<int>(std::lround(camMax_render_f));
                if (camX_render < 0) camX_render = 0;
                if (camMax_render < 0) camMax_render = 0;
                if (camX_render > camMax_render) camX_render = camMax_render;

                // Pass floating camera values to level background
                level.setBackgroundOffsetFromCamera(camX_render_f, camMax_render_f, (float)dt);
                level.updateBackground((float)dt);

                // Clear and draw: background, tiles, player, HUD
                SDL_SetRenderDrawColor(ren, 50, 50, 80, 255);
                SDL_RenderClear(ren);

                // Level background
                level.renderBackground(ren);

                // draw tiles using camX_render
                for (int r = 0; r < level.rows; ++r) {
                    for (int c = 0; c < level.cols; ++c) {
                        int cell = 0;
                        if (r >= 0 && r < (int)level.grid.size() && c >= 0 && c < (int)level.grid[r].size()) {
                            cell = level.grid[r][c];
                        }
                        if (cell == 0) continue;

                        int tileX_render = c * renderCellW - camX_render;
                        int tileY_render = r * renderCellH;
                        SDL_Rect dst{ tileX_render, tileY_render, renderCellW, renderCellH };

                        switch (cell) {
                            case 1:
                                SDL_SetRenderDrawColor(ren, 128, 128, 128, 255);
                                SDL_RenderFillRect(ren, &dst);
                                break;
                            case 2:
                            {
                                int texW, texH;
                                SDL_QueryTexture(woda.tex, nullptr, nullptr, &texW, &texH);
                                float aspect = (float)texW / texH;
                                int renderH = renderCellH;
                                int renderW = (int)(renderH * aspect + 0.5f);
                                int offsetX = (renderCellW - renderW) / 2;
                                SDL_Rect dst{ tileX_render + offsetX, tileY_render, renderW, renderH };
                                SDL_RenderCopy(ren, woda.tex, nullptr, &dst);
                            }
                                break;
                            case 3:
                            {
                                int texW, texH;
                                SDL_QueryTexture(piwo1.tex, nullptr, nullptr, &texW, &texH);
                                float aspect = (float)texW / texH;
                                int renderH = renderCellH;
                                int renderW = (int)(renderH * aspect + 0.5f);
                                int offsetX = (renderCellW - renderW) / 2;
                                SDL_Rect dst{ tileX_render + offsetX, tileY_render, renderW, renderH };
                                SDL_RenderCopy(ren, piwo1.tex, nullptr, &dst);
                            }
                                break;
                            case 4:
                            {
                                int texW, texH;
                                SDL_QueryTexture(piwo2.tex, nullptr, nullptr, &texW, &texH);
                                float aspect = (float)texW / texH;
                                int renderH = renderCellH;
                                int renderW = (int)(renderH * aspect + 0.5f);
                                int offsetX = (renderCellW - renderW) / 2;
                                SDL_Rect dst{ tileX_render + offsetX, tileY_render, renderW, renderH };
                                SDL_RenderCopy(ren, piwo2.tex, nullptr, &dst);
                            }
                                break;
                            case 5:
                                if (editMode) {
                                    SDL_SetRenderDrawColor(ren, 255, 0, 0, 255);
                                    SDL_RenderFillRect(ren, &dst);
                                }
                                break;
                            case 6:
                            {
                                int texW, texH;
                                SDL_QueryTexture(piwoKufel.tex, nullptr, nullptr, &texW, &texH);
                                float aspect = (float)texW / texH;
                                int renderH = renderCellH;
                                int renderW = (int)(renderH * aspect + 0.5f);
                                int offsetX = (renderCellW - renderW) / 2;
                                SDL_Rect dst{ tileX_render + offsetX, tileY_render, renderW, renderH };
                                SDL_RenderCopy(ren, piwoKufel.tex, nullptr, &dst);
                            }
                                break;
                            case 7:
                            {
                                int texW, texH;
                                SDL_QueryTexture(pollitroka.tex, nullptr, nullptr, &texW, &texH);
                                float aspect = (float)texW / texH;
                                int renderH = renderCellH;
                                int renderW = (int)(renderH * aspect + 0.5f);
                                int offsetX = (renderCellW - renderW) / 2;
                                SDL_Rect dst{ tileX_render + offsetX, tileY_render, renderW, renderH };
                                SDL_RenderCopy(ren, pollitroka.tex, nullptr, &dst);
                            }
                                break;
                            case 8:
                            {
                                int texW, texH;
                                SDL_QueryTexture(piwoButelka.tex, nullptr, nullptr, &texW, &texH);
                                float aspect = (float)texW / texH;
                                int renderH = renderCellH;
                                int renderW = (int)(renderH * aspect + 0.5f);
                                int offsetX = (renderCellW - renderW) / 2;
                                SDL_Rect dst{ tileX_render + offsetX, tileY_render, renderW, renderH };
                                SDL_RenderCopy(ren, piwoButelka.tex, nullptr, &dst);
                            }
                                break;
                            case 9:
                            {
                                Texture* tex = ((r + c) % 2 == 0) ? &pollitrowka3 : &pollitrowka2;
                                int texW, texH;
                                SDL_QueryTexture(tex->tex, nullptr, nullptr, &texW, &texH);
                                float aspect = (float)texW / texH;
                                int renderH = renderCellH;
                                int renderW = (int)(renderH * aspect + 0.5f);
                                int offsetX = (renderCellW - renderW) / 2;
                                SDL_Rect dst{ tileX_render + offsetX, tileY_render, renderW, renderH };
                                SDL_RenderCopy(ren, tex->tex, nullptr, &dst);
                            }
                                break;
                            case 10:
                            {
                               Texture* tex = &zelazo;
                                int texW, texH;
                                SDL_QueryTexture(tex->tex, nullptr, nullptr, &texW, &texH);
                                float aspect = (float)texW / texH;
                                int renderH = renderCellH;
                                int renderW = (int)(renderH * aspect + 0.5f);
                                int offsetX = (renderCellW - renderW) / 2;
                                SDL_Rect dst{ tileX_render + offsetX, tileY_render, renderW, renderH };
                                SDL_RenderCopy(ren, tex->tex, nullptr, &dst);
                            }
                            default:
                                SDL_SetRenderDrawColor(ren, 100, 100, 100, 255);
                                SDL_RenderFillRect(ren, &dst);
                                break;
                        }
                    }
                }

                // render player once using same camX_render
                player.render(ren, camX_render, 0, renderScale);

                // Render projectiles
                for (auto& p : projectiles) {
                    SDL_Rect dst = { (int)(p.x - camX_render), (int)(p.y), p.width, p.height };
                    SDL_RenderCopy(ren, p.tex->tex, nullptr, &dst);
                }

                // Render boss
                if (selectedLevel == 10) {
                    boss.render(ren, camX_render, 0, renderTileScale);
                }
                // render enemies
                for (auto& enemy : enemies) {
                    enemy.render(ren, camX_render, 0, renderScale);
                }

                // Render blood
                for (auto& b : bloods) {
                    SDL_SetRenderDrawColor(ren, 255, 0, 0, 255);
                     SDL_Rect dst = { (int)(b.x - camX_render), (int)b.y, 8, 8 };
                    SDL_RenderFillRect(ren, &dst);
                }

                // HUD/menu rendering
                menu.render();

                if (!editMode) {
                    SDL_Color color = {0, 0, 0, 255};
                    std::string scoreText = "Punkty: " + std::to_string(player.score);
                    std::string healthText = "HP: " + std::to_string(player.health);
                    std::string fpsText = "FPS: " + std::to_string((int)(1.0 / dt));

                    SDL_Surface* surf1 = TTF_RenderUTF8_Blended(hudFont, scoreText.c_str(), color);
                    if (surf1) {
                        SDL_Texture* tex1 = SDL_CreateTextureFromSurface(ren, surf1);
                        if (tex1) {
                            int w, h;
                            SDL_QueryTexture(tex1, nullptr, nullptr, &w, &h);
                            SDL_Rect dst = {WINW - w - 10, 10, w, h};
                            SDL_RenderCopy(ren, tex1, nullptr, &dst);
                            SDL_DestroyTexture(tex1);
                        }
                        SDL_FreeSurface(surf1);
                    }

                    SDL_Surface* surf2 = TTF_RenderUTF8_Blended(hudFont, healthText.c_str(), color);
                    if (surf2) {
                        SDL_Texture* tex2 = SDL_CreateTextureFromSurface(ren, surf2);
                        if (tex2) {
                            int w, h;
                            SDL_QueryTexture(tex2, nullptr, nullptr, &w, &h);
                            SDL_Rect dst = {10, 10, w, h};
                            SDL_RenderCopy(ren, tex2, nullptr, &dst);
                            SDL_DestroyTexture(tex2);
                        }
                        SDL_FreeSurface(surf2);
                    }

                    SDL_Color green = {0, 255, 0, 255};
                    std::string centerText;
                    SDL_Color centerColor = green;
                    if (selectedLevel == 10) {
                        centerText = "Boss HP: " + std::to_string((int)boss.hp);
                        centerColor = {255, 0, 0, 255};
                    } else {
                        centerText = currentFpsText;
                    }
                    SDL_Surface* surf3 = TTF_RenderUTF8_Blended(hudFont, centerText.c_str(), centerColor);
                    if (surf3) {
                        SDL_Texture* tex3 = SDL_CreateTextureFromSurface(ren, surf3);
                        if (tex3) {
                            int w, h;
                            SDL_QueryTexture(tex3, nullptr, nullptr, &w, &h);
                            SDL_Rect dst = {WINW / 2 - w / 2, 10, w, h};
                            SDL_RenderCopy(ren, tex3, nullptr, &dst);
                            SDL_DestroyTexture(tex3);
                        }
                        SDL_FreeSurface(surf3);
                    }
                }

                    if (editMode) {
                        SDL_Color color = {0, 0, 0, 255};
                        SDL_Surface* surf = TTF_RenderUTF8_Blended(hudFont, "Edytor: strzałki - ruch, lewy myszki - klocek (0=pusty,1=twardy,2=szkodliwy,3=bonus,5=wróg)", color);
                        if (surf) {
                            SDL_Texture* tex = SDL_CreateTextureFromSurface(ren, surf);
                            if (tex) {
                                int w, h;
                                SDL_QueryTexture(tex, nullptr, nullptr, &w, &h);
                                SDL_Rect dst = {10, 10, w, h};
                                SDL_RenderCopy(ren, tex, nullptr, &dst);
                                SDL_DestroyTexture(tex);
                            }
                            SDL_FreeSurface(surf);
                        }
                    }

                    // Render game over screens
                    if (playerLost) {
                        fade += (float)dt * 200.0f; // fade in
                        if (fade > 255.0f) fade = 255.0f;

                        SDL_SetRenderDrawColor(ren, 0, 0, 0, (Uint8)fade);
                        SDL_RenderFillRect(ren, nullptr);

                        SDL_Color color = {255, 0, 0, 255};
                        SDL_Surface* surf = TTF_RenderUTF8_Blended(hudFont, "Przegrałeś", color);
                        if (surf) {
                            SDL_Texture* tex = SDL_CreateTextureFromSurface(ren, surf);
                            if (tex) {
                                int w, h;
                                SDL_QueryTexture(tex, nullptr, nullptr, &w, &h);
                                SDL_Rect dst = {WINW / 2 - w / 2, WINH / 2 - h / 2, w, h};
                                SDL_RenderCopy(ren, tex, nullptr, &dst);
                                SDL_DestroyTexture(tex);
                            }
                            SDL_FreeSurface(surf);
                        }
                    } else if (playerWon) {
                        SDL_SetRenderDrawColor(ren, 102, 51, 153, 255);
                        SDL_RenderFillRect(ren, nullptr);
                        SDL_Color color = {255, 215, 0, 255};
                        SDL_Surface* surf = TTF_RenderUTF8_Blended(hudFont, "Wygrałeś", color);
                        if (surf) {
                            SDL_Texture* tex = SDL_CreateTextureFromSurface(ren, surf);
                            if (tex) {
                                int w, h;
                                SDL_QueryTexture(tex, nullptr, nullptr, &w, &h);
                                SDL_Rect dst = {WINW / 2 - w / 2, WINH / 2 - h / 2, w, h};
                                SDL_RenderCopy(ren, tex, nullptr, &dst);
                                SDL_DestroyTexture(tex);
                            }
                            SDL_FreeSurface(surf);
                        }
                    }

                    SDL_RenderPresent(ren);
                    SDL_Delay(5);
                }

                // Halt music

                // Wait for enter to return to menu
                bool waiting = true;
                while (waiting) {
                    SDL_Event ev;
                    while (SDL_PollEvent(&ev)) {
                        if (ev.type == SDL_QUIT) { waiting = false; break; }
                        if (ev.type == SDL_KEYDOWN && ev.key.keysym.scancode == SDL_SCANCODE_RETURN) {
                            waiting = false;
                            break;
                        }
                    }

                    // Re-render the last screen
                    if (playerLost) {
                        SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
                        SDL_RenderFillRect(ren, nullptr);
                        SDL_Color color = {255, 0, 0, 255};
                        SDL_Surface* surf = TTF_RenderUTF8_Blended(hudFont, "Przegrałeś", color);
                        if (surf) {
                            SDL_Texture* tex = SDL_CreateTextureFromSurface(ren, surf);
                            if (tex) {
                                int w, h;
                                SDL_QueryTexture(tex, nullptr, nullptr, &w, &h);
                                SDL_Rect dst = {WINW / 2 - w / 2, WINH / 2 - h / 2, w, h};
                                SDL_RenderCopy(ren, tex, nullptr, &dst);
                                SDL_DestroyTexture(tex);
                            }
                            SDL_FreeSurface(surf);
                        }
                    } else if (playerWon) {
                        SDL_SetRenderDrawColor(ren, 102, 51, 153, 255);
                        SDL_RenderFillRect(ren, nullptr);
                        SDL_Color color = {255, 215, 0, 255};
                        SDL_Surface* surf = TTF_RenderUTF8_Blended(hudFont, "Wygrałeś", color);
                        if (surf) {
                            SDL_Texture* tex = SDL_CreateTextureFromSurface(ren, surf);
                            if (tex) {
                                int w, h;
                                SDL_QueryTexture(tex, nullptr, nullptr, &w, &h);
                                SDL_Rect dst = {WINW / 2 - w / 2, WINH / 2 - h / 2, w, h};
                                SDL_RenderCopy(ren, tex, nullptr, &dst);
                                SDL_DestroyTexture(tex);
                            }
                            SDL_FreeSurface(surf);
                        }
                    }

                    SDL_RenderPresent(ren);
                    SDL_Delay(16);
                }

                // Halt music again if needed
                Mix_HaltMusic();

                // Save progress if level completed
                if (playerWon) {
                    saveData.completedLevels |= (1 << selectedLevel);
                    saveProgress(saveData);
                }

                // Cleanup for this level
                delete editor;
                editor = nullptr;
            }

            // cleanup
            if(hudFont) TTF_CloseFont(hudFont);
            Mix_FreeMusic(menuMusic);
            Mix_FreeMusic(levelMusic);
            Mix_FreeMusic(bossMusic);
            Mix_FreeChunk(deadSound);
            Mix_FreeChunk(pickSound);
            Mix_FreeChunk(stepSound);
            TTF_Quit();
            IMG_Quit();
            Mix_CloseAudio();
            SDL_Quit();
            return 0;
        }
