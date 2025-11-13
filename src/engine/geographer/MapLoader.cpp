#include "MapLoader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include "resource_manager/TextureNames.h"
#include "resource_manager/MusicNames.h"

namespace fs = std::filesystem;

LevelResources MapLoader::loadLevel(const std::string& filename) {
    LevelResources resources;
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        std::cerr << "Failed to load level file: " << filename << std::endl;
        return createTestLevel();
    }
    
    std::string line;
    bool readingGrid = false;
    std::vector<std::vector<int>> grid;
    
    while (std::getline(file, line)) {
        // Пропускаем пустые строки и комментарии
        if (line.empty() || line[0] == '#') continue;
        
        // Обрабатываем секции
        if (line == "[GRID]") {
            readingGrid = true;
            continue;
        } else if (line == "[RESOURCES]") {
            readingGrid = false;
            continue;
        } else if (line == "[PLAYER]") {
            readingGrid = false;
            continue;
        }
        
        if (readingGrid) {
            // Чтение сетки уровня
            std::vector<int> row;
            std::istringstream iss(line);
            int value;
            
            while (iss >> value) {
                row.push_back(value);
            }
            
            if (!row.empty()) {
                grid.push_back(row);
            }
        } else {
            // Обработка ресурсов и позиции игрока
            if (!parseResourceLine(line, resources) && !parsePlayerPosition(line, resources)) {
                std::cout << "Unknown level directive: " << line << std::endl;
            }
        }
    }
    
    resources.grid = grid;
    file.close();
    
    // Загружаем ресурсы через ResourceManager
    auto& rm = ResourceManager::getInstance();
    
    // Загрузка текстур
    for (const auto& [textureName, texturePath] : resources.textures) {
        if (fs::exists(texturePath)) {
            rm.loadTexture(textureName, texturePath);
            std::cout << "Loaded texture for level: " << textureName << std::endl;
        } else {
            std::cerr << "Texture file not found: " << texturePath << std::endl;
        }
    }
    
    // Загрузка музыки
    if (!resources.backgroundMusic.empty() && fs::exists(resources.backgroundMusic)) {
        rm.loadMusic(Music::ADRIANS_ASLEEP, resources.backgroundMusic);
        std::cout << "Loaded music for level: " << resources.backgroundMusic << std::endl;
    }
    
    return resources;
}

bool MapLoader::parseResourceLine(const std::string& line, LevelResources& resources) {
    std::istringstream iss(line);
    std::string type, name, path;
    
    if (!(iss >> type >> name >> path)) {
        return false;
    }
    
    if (type == "texture") {
        resources.textures[name] = path;
        return true;
    } else if (type == "music") {
        resources.backgroundMusic = path;
        return true;
    }
    
    return false;
}

bool MapLoader::parsePlayerPosition(const std::string& line, LevelResources& resources) {
    std::istringstream iss(line);
    std::string type;
    float x, y, dirX, dirY;
    
    if (iss >> type >> x >> y >> dirX >> dirY && type == "player") {
        resources.playerStartPosition = sf::Vector2f(x, y);
        resources.playerStartDirection = sf::Vector2f(dirX, dirY);
        return true;
    }
    
    return false;
}

LevelResources MapLoader::createTestLevel() {
    LevelResources resources;
    
    // Тестовая сетка
    resources.grid = {
        {1, 1, 1, 1, 1, 1, 1, 1},
        {1, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 1},
        {1, 1, 1, 1, 1, 1, 1, 1}
    };
    
    // Пути к ресурсам для тестового уровня
    std::vector<std::string> possibleTexturePaths = {
        "resources/textures/surfaces/",
        "../resources/textures/surfaces/", 
        "../../resources/textures/surfaces/",
        "../../../resources/textures/surfaces/",
        "textures/surfaces/",
        "../textures/surfaces/"
    };
    
    std::vector<std::string> possibleMusicPaths = {
        "resources/sounds/music/",
        "../resources/sounds/music/",
        "../../resources/sounds/music/", 
        "../../../resources/sounds/music/",
        "sounds/music/",
        "../sounds/music/"
    };
    
    // Поиск текстур
    for (const auto& path : possibleTexturePaths) {
        std::string bricksPath = path + "bricks.png";
        if (fs::exists(bricksPath)) {
            resources.textures[Textures::BRICKS] = bricksPath;
            resources.textures[Textures::BOARDS] = path + "boards.png";
            resources.textures[Textures::PARQUET] = path + "parquet.png";
            break;
        }
    }
    
    // Поиск музыки
    for (const auto& path : possibleMusicPaths) {
        std::vector<std::string> possibleMusicFiles = {
            "Adrian'sAsleep.mid"
        };
        
        for (const auto& musicFile : possibleMusicFiles) {
            std::string musicPath = path + musicFile;
            if (fs::exists(musicPath)) {
                resources.backgroundMusic = musicPath;
                std::cout << "Found music file: " << musicPath << std::endl;
                break;
            }
        }
        
        if (!resources.backgroundMusic.empty()) {
            break;
        }
    }
    
    // Стартовая позиция игрока
    resources.playerStartPosition = sf::Vector2f(1.5f, 1.5f);
    resources.playerStartDirection = sf::Vector2f(-1.0f, 0.0f);
    
    return resources;
}