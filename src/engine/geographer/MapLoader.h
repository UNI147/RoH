#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include "resource_manager/ResourceManager.h"

class MapLoader {
public:
    // Загрузка и парсинг уровня из файла
    static std::unique_ptr<LevelData> loadLevelFromFile(const std::string& filename);
    
    // Сохранение уровня в файл (если нужно)
    static bool saveLevelToFile(const LevelData& level, const std::string& filename);
    
    // Валидация структуры уровня
    static bool validateLevel(const LevelData& level);
    
    // Генерация уникального имени уровня
    static std::string generateLevelName();

private:
    // Вспомогательные методы для парсинга
    static bool parseResourceLine(const std::string& line, LevelData& level);
    static bool parsePlayerPosition(const std::string& line, LevelData& level);
    static bool parseLevelInfo(const std::string& line, LevelData& level);
    static bool parseGridLine(const std::string& line, std::vector<std::vector<int>>& grid);
};
