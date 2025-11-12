#pragma once
#include <SFML/Graphics.hpp>
#include "../raycasting/RayCaster.h"

class Renderer {
public:
    Renderer(sf::RenderWindow& window);
    void renderFrame(const PlayerState& player, const std::vector<std::vector<int>>& map,
                    RayCaster& rayCaster);

private:
    sf::RenderWindow& window_;
    
    void drawWallStrip(int x, int lineHeight, int side, float wallX);
    void drawFloorAndCeiling(int x, int drawStart, int drawEnd);
    
    const int RENDER_WIDTH = 320;
    const int RENDER_HEIGHT = 200;
    
    sf::RenderTexture renderTexture_;
    sf::Sprite renderSprite_;
    
    void updateRenderSpriteScale();
};