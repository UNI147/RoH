#pragma once
#include <SFML/System/Vector2.hpp>
#include <vector>

struct PlayerState {
    sf::Vector2f position;
    sf::Vector2f direction;
    sf::Vector2f plane;
};

struct RayHit {
    float distance;
    int mapX, mapY;
    int side;
    float wallX;
};

class RayCaster {
public:
    RayHit castRay(const PlayerState& player, const std::vector<std::vector<int>>& map, 
                   float cameraX) const;
    
private:
    bool isWall(const std::vector<std::vector<int>>& map, int x, int y) const;
};