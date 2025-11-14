#include "MapLoader.h"

bool MapLoader::loadLevel(const std::string& levelName, const std::string& filename) {
    auto& rm = ResourceManager::getInstance();
    return rm.loadLevel(levelName, filename);
}

const LevelData& MapLoader::getLevel(const std::string& levelName) {
    auto& rm = ResourceManager::getInstance();
    return rm.getLevel(levelName);
}