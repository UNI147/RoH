#pragma once
#include <SFML/Graphics.hpp>
#include "engine/raycasting/RayCaster.h"

class Renderer {
public:
    Renderer(sf::RenderWindow& window);
    void renderFrame(const PlayerState& player, const std::vector<std::vector<int>>& map,
                    RayCaster& rayCaster);

private:
    sf::RenderWindow& window_;
    
    void drawWallStrip(int x, int lineHeight, int side, float wallX);
    void drawFloorAndCeiling(int x, int drawStart, int drawEnd);
    
    const int SCREEN_WIDTH = 800;
    const int SCREEN_HEIGHT = 600;
};