#pragma once
#include <string>

bool saveLevelZip(const std::string& zipPath, const std::string& layout, const std::string& assets, const std::string& backgroundPath);
bool loadLevelZip(const std::string& zipPath, std::string& layout, std::string& assets);
