#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include "resource_manager/ResourceManager.h"
#include "sound_engineer/SoundEngineer.h"

struct LevelResources {
    std::vector<std::vector<int>> grid;
    std::unordered_map<std::string, std::string> textures;
    std::string backgroundMusic;
    sf::Vector2f playerStartPosition;
    sf::Vector2f playerStartDirection;
};

class MapLoader {
public:
    LevelResources loadLevel(const std::string& filename);
    LevelResources createTestLevel();
    
private:
    bool parseResourceLine(const std::string& line, LevelResources& resources);
    bool parsePlayerPosition(const std::string& line, LevelResources& resources);
};