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
    
    void drawWallStrip(int x, int drawStart, int drawEnd, int side, float distance);
    void drawFloorAndCeiling(int x, int drawStart, int drawEnd);
    void drawTexturedWallStrip(int x, int drawStart, int drawEnd, const RayHit& hit, int lineHeight);
    void drawTexturedFloorAndCeiling(const PlayerState& player, const std::vector<std::vector<int>>& map,
                                    RayCaster& rayCaster);
    void drawSolidFloorAndCeiling();
    
    static const int RENDER_WIDTH = 320;
    static const int RENDER_HEIGHT = 200;
    
    sf::RenderTexture renderTexture_;
    sf::Sprite renderSprite_;
    
    // Предзагруженные изображения текстур для производительности
    sf::Image wallTextureImage_;
    sf::Image floorTextureImage_;
    sf::Image ceilingTextureImage_;
    
    void updateRenderSpriteScale();
    
    bool useTextures_ = true;
};