#pragma once
#include <vector>
#include <string>

class MapLoader {
public:
    std::vector<std::vector<int>> loadMap(const std::string& filename);
    std::vector<std::vector<int>> createTestMap();
};