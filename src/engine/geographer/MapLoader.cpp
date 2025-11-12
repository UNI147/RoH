#include "MapLoader.h"
#include <fstream>
#include <sstream>
#include <iostream>

std::vector<std::vector<int>> MapLoader::loadMap(const std::string& filename) {
    std::vector<std::vector<int>> map;
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        std::cerr << "Failed to load map file: " << filename << std::endl;
        return createTestMap(); // Возвращаем тестовую карту если файл не найден
    }
    
    std::string line;
    while (std::getline(file, line)) {
        std::vector<int> row;
        std::istringstream iss(line);
        int value;
        
        while (iss >> value) {
            row.push_back(value);
        }
        
        if (!row.empty()) {
            map.push_back(row);
        }
    }
    
    file.close();
    return map;
}

std::vector<std::vector<int>> MapLoader::createTestMap() {
    // Создаем тестовую карту
    return {
        {1, 1, 1, 1, 1, 1, 1, 1},
        {1, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 1, 0, 0, 1, 0, 1},
        {1, 0, 1, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 1, 0, 1},
        {1, 0, 1, 1, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 1},
        {1, 1, 1, 1, 1, 1, 1, 1}
    };
}