#include "MapLoader.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <ctime>
#include "../geographer/Furniture.h"
#include "../geographer/FurnitureLoader.h"

namespace fs = std::filesystem;

static bool parseFurnitureTypeLine(const std::string& line, LevelData& level) {
    std::istringstream iss(line);
    std::string furnitureName;
    std::string furnitureFile;
    
    if (iss >> furnitureName >> furnitureFile) {
        // Загружаем данные мебели из .fur файла
        auto furData = FurnitureLoader::loadFurnitureFromFile(furnitureFile);
        if (furData) {
            level.furnitureTypes[furnitureName] = *furData;
            
            // Отладочный вывод
            std::cout << "Loaded furniture type: " << furnitureName 
                      << " from " << furnitureFile 
                      << " (texture: " << furData->textureName << ")" << std::endl;
            return true;
        } else {
            std::cerr << "Failed to load furniture type: " << furnitureName 
                      << " from " << furnitureFile << std::endl;
        }
    }
    return false;
}

static bool parseFurnitureObjectLine(const std::string& line, LevelData& level) {
    std::istringstream iss(line);
    std::string type;
    float x, y, rotation = 0.0f, scale = 1.0f;
    
    if (iss >> type >> x >> y) {
        FurnitureObject obj;
        obj.furnitureType = type;
        obj.position = sf::Vector2f(x, y);
        
        // Опциональные параметры
        iss >> rotation;
        if (!iss.fail()) obj.rotation = rotation;
        
        iss >> scale;
        if (!iss.fail()) obj.scale = scale;
        
        level.furnitureObjects.push_back(obj);
        return true;
    }
    return false;
}

bool MapLoader::parseFurnitureLine(const std::string& line, LevelData& level) {
    std::istringstream iss(line);
    std::string type;
    std::string furnitureName;
    float x, y, rotation = 0.0f, scale = 1.0f;
    
    // Формат: furniture_type x y [rotation] [scale]
    if (iss >> type >> furnitureName >> x >> y) {
        FurnitureObject obj;
        obj.furnitureType = furnitureName;
        obj.position = sf::Vector2f(x, y);
        
        // Опциональные параметры
        iss >> rotation;
        if (!iss.fail()) obj.rotation = rotation;
        
        iss >> scale;
        if (!iss.fail()) obj.scale = scale;
        
        // Опциональный цвет
        float r, g, b, a = 255.0f;
        if (iss >> r >> g >> b) {
            iss >> a;
            if (iss.fail()) a = 255.0f;
            obj.tint = sf::Color(
                static_cast<sf::Uint8>(r),
                static_cast<sf::Uint8>(g),
                static_cast<sf::Uint8>(b),
                static_cast<sf::Uint8>(a)
            );
        }
        
        level.furnitureObjects.push_back(obj);
        return true;
    }
    
    return false;
}

std::unique_ptr<LevelData> MapLoader::loadLevelFromFile(const std::string& filename) {
    auto levelData = std::make_unique<LevelData>();
    
    auto& rm = ResourceManager::getInstance();
    std::string foundPath = rm.findResourceFile(filename);
    
    if (foundPath.empty()) {
        std::cerr << "Level file not found: " << filename << std::endl;
        return nullptr;
    }
    
    std::ifstream file(foundPath);
    if (!file.is_open()) {
        std::cerr << "Failed to open level file: " << foundPath << std::endl;
        return nullptr;
    }
    
    std::string line;
    std::string currentSection;
    std::vector<std::vector<int>> currentGrid;
    
    // Инициализируем имя уровня из имени файла
    levelData->name = fs::path(filename).stem().string();
    
    while (std::getline(file, line)) {
        // Пропускаем пустые строки и комментарии
        if (line.empty() || line[0] == '#') continue;
        
        // Обрабатываем секции
        if (line[0] == '[' && line[line.size() - 1] == ']') {
            // Сохраняем предыдущую сетку
            if (currentSection == "[WALL_LAYER]" && !currentGrid.empty()) {
                levelData->wallGrid = currentGrid;
            } else if (currentSection == "[FLOOR_LAYER]" && !currentGrid.empty()) {
                levelData->floorGrid = currentGrid;
            } else if (currentSection == "[CEILING_LAYER]" && !currentGrid.empty()) {
                levelData->ceilingGrid = currentGrid;
            }
            
            currentSection = line;
            currentGrid.clear();
            continue;
        }
        
        // Парсим строку в зависимости от текущей секции
        if (currentSection == "[LEVEL]") {
            parseLevelInfo(line, *levelData);
        } 
        else if (currentSection == "[WALL_TEXTURES]") {
            parseTextureLine(line, *levelData, "wall");
        } 
        else if (currentSection == "[FLOOR_TEXTURES]") {
            parseTextureLine(line, *levelData, "floor");
        } 
        else if (currentSection == "[CEILING_TEXTURES]") {
            parseTextureLine(line, *levelData, "ceiling");
        } 
        else if (currentSection == "[PLAYER]") {
            parsePlayerPosition(line, *levelData);
        } 
        else if (currentSection == "[FURNITURE_TYPES]") {
            parseFurnitureTypeLine(line, *levelData);
        } 
        else if (currentSection == "[FURNITURE_OBJECTS]") {
            if (!parseFurnitureObjectLine(line, *levelData)) {
                std::cerr << "Failed to parse furniture object line: " << line << std::endl;
            }
        }
        else if (currentSection == "[WALL_LAYER]" || 
                 currentSection == "[FLOOR_LAYER]" || 
                 currentSection == "[CEILING_LAYER]") {
            parseGridLine(line, currentGrid);
        }
    }
    
    // Сохраняем последнюю сетку
    if (currentSection == "[WALL_LAYER]" && !currentGrid.empty()) {
        levelData->wallGrid = currentGrid;
    } else if (currentSection == "[FLOOR_LAYER]" && !currentGrid.empty()) {
        levelData->floorGrid = currentGrid;
    } else if (currentSection == "[CEILING_LAYER]" && !currentGrid.empty()) {
        levelData->ceilingGrid = currentGrid;
    }
    
    file.close();
    
    // Валидация загруженного уровня
    if (!validateLevel(*levelData)) {
        std::cerr << "Level validation failed: " << filename << std::endl;
        return nullptr;
    }
    
    std::cout << "Successfully parsed level: " << levelData->name 
              << " from " << foundPath << std::endl;
    std::cout << "Wall grid size: " << levelData->wallGrid.size() << "x" 
              << (levelData->wallGrid.empty() ? 0 : levelData->wallGrid[0].size()) << std::endl;
    std::cout << "Floor grid size: " << levelData->floorGrid.size() << "x" 
              << (levelData->floorGrid.empty() ? 0 : levelData->floorGrid[0].size()) << std::endl;
    std::cout << "Ceiling grid size: " << levelData->ceilingGrid.size() << "x" 
              << (levelData->ceilingGrid.empty() ? 0 : levelData->ceilingGrid[0].size()) << std::endl;
    
    return levelData;
}

bool MapLoader::saveLevelToFile(const LevelData& level, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to create level file: " << filename << std::endl;
        return false;
    }
    
    file << "# RoH Level File с тремя текстурными слоями\n";
    file << "# Формат: [SECTION] затем параметры\n\n";
    
    // Секция LEVEL
    file << "[LEVEL]\n";
    file << "name \"" << level.name << "\"\n";
    file << "background_music " << level.backgroundMusic << "\n";
    file << "ambience_sound " << level.ambienceSound << "\n\n";
    
    // Секция WALL_TEXTURES
    file << "[WALL_TEXTURES]\n";
    file << "# Формат: ID путь_к_текстуре\n";
    for (const auto& [id, texName] : level.wallTextures) {
        if (level.texturePaths.find(texName) != level.texturePaths.end()) {
            file << id << " " << level.texturePaths.at(texName) << "\n";
        }
    }
    file << "\n";
    
    // Секция FLOOR_TEXTURES
    file << "[FLOOR_TEXTURES]\n";
    file << "# ID путь_к_текстуре\n";
    for (const auto& [id, texName] : level.floorTextures) {
        if (level.texturePaths.find(texName) != level.texturePaths.end()) {
            file << id << " " << level.texturePaths.at(texName) << "\n";
        }
    }
    file << "\n";
    
    // Секция CEILING_TEXTURES
    file << "[CEILING_TEXTURES]\n";
    file << "# ID путь_к_текстуре\n";
    for (const auto& [id, texName] : level.ceilingTextures) {
        if (level.texturePaths.find(texName) != level.texturePaths.end()) {
            file << id << " " << level.texturePaths.at(texName) << "\n";
        }
    }
    file << "\n";
    
    // Секция PLAYER
    file << "[PLAYER]\n";
    file << "player " << level.playerStartPosition.x << " " << level.playerStartPosition.y
         << " " << level.playerStartDirection.x << " " << level.playerStartDirection.y << "\n\n";
    
    // Секция WALL_LAYER
    file << "[WALL_LAYER]\n";
    for (const auto& row : level.wallGrid) {
        for (size_t i = 0; i < row.size(); ++i) {
            file << row[i];
            if (i < row.size() - 1) file << " ";
        }
        file << "\n";
    }
    file << "\n";
    
    // Секция FLOOR_LAYER
    file << "[FLOOR_LAYER]\n";
    for (const auto& row : level.floorGrid) {
        for (size_t i = 0; i < row.size(); ++i) {
            file << row[i];
            if (i < row.size() - 1) file << " ";
        }
        file << "\n";
    }
    file << "\n";
    
    // Секция CEILING_LAYER
    file << "[CEILING_LAYER]\n";
    for (const auto& row : level.ceilingGrid) {
        for (size_t i = 0; i < row.size(); ++i) {
            file << row[i];
            if (i < row.size() - 1) file << " ";
        }
        file << "\n";
    }
    
    file.close();
    std::cout << "Level saved to: " << filename << std::endl;
    return true;
}

bool MapLoader::validateLevel(const LevelData& level) {
    // Проверяем основные поля
    if (level.name.empty()) {
        std::cerr << "Level validation error: Empty level name" << std::endl;
        return false;
    }
    
    if (level.wallGrid.empty()) {
        std::cerr << "Level validation error: Empty wall grid" << std::endl;
        return false;
    }
    
    // Проверяем размеры всех трех сеток (должны быть одинаковыми)
    size_t gridHeight = level.wallGrid.size();
    size_t gridWidth = level.wallGrid[0].size();
    
    if (level.floorGrid.size() != gridHeight || level.ceilingGrid.size() != gridHeight) {
        std::cerr << "Level validation error: Grids have different heights" << std::endl;
        return false;
    }
    
    for (size_t i = 0; i < gridHeight; ++i) {
        if (level.wallGrid[i].size() != gridWidth ||
            level.floorGrid[i].size() != gridWidth ||
            level.ceilingGrid[i].size() != gridWidth) {
            std::cerr << "Level validation error: Grid rows have different sizes at row " << i << std::endl;
            return false;
        }
    }
    
    // Проверяем стартовую позицию игрока
    int gridX = static_cast<int>(level.playerStartPosition.x);
    int gridY = static_cast<int>(level.playerStartPosition.y);
    
    if (gridY < 0 || gridY >= static_cast<int>(gridHeight) ||
        gridX < 0 || gridX >= static_cast<int>(gridWidth)) {
        std::cerr << "Level validation error: Player start position outside grid" << std::endl;
        return false;
    }
    
    if (level.wallGrid[gridY][gridX] != 0) {
        std::cerr << "Level validation error: Player starts inside wall" << std::endl;
        return false;
    }
    
    // Проверяем направление игрока
    float dirLength = std::sqrt(
        level.playerStartDirection.x * level.playerStartDirection.x +
        level.playerStartDirection.y * level.playerStartDirection.y
    );
    
    if (dirLength < 0.01f) {
        std::cerr << "Level validation error: Player direction is zero vector" << std::endl;
        return false;
    }
    
    return true;
}

// Добавляем новый метод для парсинга текстурных строк
bool MapLoader::parseTextureLine(const std::string& line, LevelData& level, const std::string& type) {
    std::istringstream iss(line);
    int id;
    std::string path;
    
    if (iss >> id >> path) {
        // Создаем уникальное имя текстуры на основе типа и ID
        std::string texName = type + "_" + std::to_string(id);
        
        // Сохраняем сопоставление ID -> имя текстуры
        if (type == "wall") {
            level.wallTextures[id] = texName;
        } else if (type == "floor") {
            level.floorTextures[id] = texName;
        } else if (type == "ceiling") {
            level.ceilingTextures[id] = texName;
        }
        
        // Сохраняем путь к текстуре
        level.texturePaths[texName] = path;
        
        return true;
    }
    
    return false;
}

bool MapLoader::parseLevelInfo(const std::string& line, LevelData& level) {
    std::istringstream iss(line);
    std::string key;
    
    if (!(iss >> key)) {
        return false;
    }
    
    if (key == "name") {
        // Имя в кавычках
        std::string name;
        std::getline(iss, name);
        // Убираем кавычки и лишние пробелы
        if (name.size() >= 2 && name[0] == '"' && name[name.size()-1] == '"') {
            name = name.substr(1, name.size() - 2);
        }
        // Убираем начальные пробелы
        name.erase(0, name.find_first_not_of(" \t"));
        level.name = name;
        return true;
    } else if (key == "background_music") {
        std::string musicPath;
        iss >> musicPath;
        level.backgroundMusic = musicPath;
        return true;
    } else if (key == "ambience_sound") {
        std::string soundPath;
        iss >> soundPath;
        level.ambienceSound = soundPath;
        return true;
    }
    
    return false;
}

bool MapLoader::parsePlayerPosition(const std::string& line, LevelData& level) {
    std::istringstream iss(line);
    std::string type;
    float x, y, dirX, dirY;
    
    if (iss >> type >> x >> y >> dirX >> dirY && type == "player") {
        level.playerStartPosition = sf::Vector2f(x, y);
        level.playerStartDirection = sf::Vector2f(dirX, dirY);
        return true;
    }
    
    return false;
}

bool MapLoader::parseGridLine(const std::string& line, std::vector<std::vector<int>>& grid) {
    std::vector<int> row;
    std::istringstream iss(line);
    int value;
    
    while (iss >> value) {
        row.push_back(value);
    }
    
    if (!row.empty()) {
        grid.push_back(row);
        return true;
    }
    
    return false;
}

std::string MapLoader::generateLevelName() {
    std::time_t now = std::time(nullptr);
    std::tm localTime;
    
    localtime_s(&localTime, &now);
    
    char buffer[64];
    std::strftime(buffer, sizeof(buffer), "level_%Y%m%d_%H%M%S", &localTime);
    
    return std::string(buffer);
}