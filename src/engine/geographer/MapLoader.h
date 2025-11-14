#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include "resource_manager/ResourceManager.h"
#include "sound_engineer/SoundEngineer.h"

class MapLoader {
public:
    // Используем только статические методы
    MapLoader() = default;
    
    // Загружает уровень
    static bool loadLevel(const std::string& levelName, const std::string& filename);
    
    // Получить загруженный уровень
    static const LevelData& getLevel(const std::string& levelName);
};