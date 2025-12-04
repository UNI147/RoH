#include "FurnitureLoader.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include "resource_manager/ResourceManager.h"

namespace fs = std::filesystem;

std::unique_ptr<FurnitureData> FurnitureLoader::loadFurnitureFromFile(const std::string& filename) {
    auto furniture = std::make_unique<FurnitureData>();
    
    // Используем ResourceManager для поиска файла
    std::string foundPath = ResourceManager::getInstance().findResourceFile(filename);
    if (foundPath.empty()) {
        std::cerr << "Furniture file not found: " << filename << std::endl;
        return nullptr;
    }
    
    std::ifstream file(foundPath);
    if (!file.is_open()) {
        std::cerr << "Failed to open furniture file: " << filename << std::endl;
        return nullptr;
    }
    
    std::string line;
    furniture->name = fs::path(filename).stem().string();
    
    while (std::getline(file, line)) {
        // Пропускаем пустые строки и комментарии
        if (line.empty() || line[0] == '#') continue;
        
        parseFurnitureLine(line, *furniture);
    }
    
    file.close();
    
    std::cout << "Loaded furniture: " << furniture->name 
              << " (texture: " << furniture->textureName << ")" << std::endl;
    
    return furniture;
}

bool FurnitureLoader::parseFurnitureLine(const std::string& line, FurnitureData& furniture) {
    std::istringstream iss(line);
    std::string key;
    
    if (!(iss >> key)) {
        return false;
    }
    
    if (key == "texture") {
        std::string texture;
        iss >> texture;
        furniture.textureName = texture;
        return true;
    }
    else if (key == "width") {
        float width;
        if (iss >> width) furniture.width = width;
        return true;
    }
    else if (key == "height") {
        float height;
        if (iss >> height) furniture.height = height;
        return true;
    }
    else if (key == "passable") {
        std::string value;
        if (iss >> value) furniture.passable = (value == "true" || value == "1");
        return true;
    }
    else if (key == "transparent") {
        std::string value;
        if (iss >> value) furniture.transparent = (value == "true" || value == "1");
        return true;
    }
    else if (key == "y_offset") {
        float offset;
        if (iss >> offset) furniture.yOffset = offset;
        return true;
    }
    else if (key == "pivot") {
        float x, y;
        if (iss >> x >> y) furniture.pivot = {x, y};
        return true;
    }
    else if (key == "collision_box") {
        float x, y, w, h;
        if (iss >> x >> y >> w >> h) {
            furniture.collisionBox.x = x;
            furniture.collisionBox.y = y;
            furniture.collisionBox.width = w;
            furniture.collisionBox.height = h;
        }
        return true;
    }
    
    return false;
}

bool FurnitureLoader::saveFurnitureToFile(const FurnitureData& furniture, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    file << "# Furniture definition file\n";
    file << "# Format: key value(s)\n\n";
    
    file << "texture " << furniture.textureName << "\n";
    file << "width " << furniture.width << "\n";
    file << "height " << furniture.height << "\n";
    file << "passable " << (furniture.passable ? "true" : "false") << "\n";
    file << "transparent " << (furniture.transparent ? "true" : "false") << "\n";
    file << "y_offset " << furniture.yOffset << "\n";
    file << "pivot " << furniture.pivot.x << " " << furniture.pivot.y << "\n";
    file << "collision_box " << furniture.collisionBox.x << " " 
         << furniture.collisionBox.y << " " 
         << furniture.collisionBox.width << " " 
         << furniture.collisionBox.height << "\n";
    
    file.close();
    return true;
}