#pragma once
#include "resource_manager/ResourceManager.h"
#include "engine/geographer/MapLoader.h"
#include "sound_engineer/SoundEngineer.h"
#include <memory>
#include <string>

class LevelProcessor {
public:
    // Загрузка и обработка уровня
    static bool processLevel(const std::string& levelName, 
                           const std::string& filename,
                           SoundEngineer& soundEngineer);
    
    // Получение обработанного уровня
    static const LevelData& getProcessedLevel(const std::string& levelName);
    
    // Освобождение ресурсов уровня
    static void unloadLevel(const std::string& levelName);
    
    // Загрузка ресурсов уровня
    static bool loadLevelResources(const LevelData& level);
    
    // Настройка звуков уровня
    static bool setupLevelSounds(const LevelData& level, SoundEngineer& soundEngineer);
    
    // Проверка готовности уровня
    static bool isLevelReady(const std::string& levelName);
    
    // Очистка всех загруженных уровней
    static void clearAllLevels();

private:
    // Загрузка текстур уровня
    static bool loadLevelTextures(const LevelData& level);
    
    // Загрузка музыки уровня
    static bool loadLevelMusic(const LevelData& level);
    
    // Загрузка звуков уровня
    static bool loadLevelSounds(const LevelData& level);
};