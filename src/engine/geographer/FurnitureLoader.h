#pragma once
#include "Furniture.h"
#include <string>
#include <memory>

class FurnitureLoader {
public:
    static std::unique_ptr<FurnitureData> loadFurnitureFromFile(const std::string& filename);
    static bool saveFurnitureToFile(const FurnitureData& furniture, const std::string& filename);
    
private:
    static bool parseFurnitureLine(const std::string& line, FurnitureData& furniture);
};