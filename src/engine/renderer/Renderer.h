#pragma once
#include <SFML/Graphics.hpp>
#include <unordered_map>
#include "../raycasting/RayCaster.h"

class Renderer {
public:
    Renderer(sf::RenderWindow& window);
    
    // Основной метод рендеринга с тремя текстурными слоями
    void renderFrame(const PlayerState& player, 
                     const std::vector<std::vector<int>>& wallMap,
                     const std::vector<std::vector<int>>& floorMap,
                     const std::vector<std::vector<int>>& ceilingMap,
                     RayCaster& rayCaster);
    
    void setWallTexture(int wallType, const std::string& textureName);
    
    // Методы для управления высотой камеры (точки обзора)
    void setViewHeight(float height);
    float getViewHeight() const;
    
    void setFloorTexture(const std::string& textureName);
    void setCeilingTexture(const std::string& textureName);
    void addWallTexture(const std::string& textureName);
    
    // Новые методы для управления текстурными слоями
    void addWallTexture(int textureId, const std::string& textureName);
    void addFloorTexture(int textureId, const std::string& textureName);
    void addCeilingTexture(int textureId, const std::string& textureName);

private:
    sf::RenderWindow& window_;
    
    void drawWallStrip(int x, int drawStart, int drawEnd, int side, float distance);
    void drawFloorAndCeiling(int x, int drawStart, int drawEnd);
    void drawTexturedWallStrip(int x, int drawStart, int drawEnd, 
                              const RayHit& hit, int lineHeight,
                              const std::vector<std::vector<int>>& wallMap);
    void drawTexturedFloorAndCeiling(const PlayerState& player, 
                                    const std::vector<std::vector<int>>& floorMap,
                                    const std::vector<std::vector<int>>& ceilingMap,
                                    RayCaster& rayCaster);
    void drawSolidFloorAndCeiling();
    
    static const int RENDER_WIDTH = 320;
    static const int RENDER_HEIGHT = 200;
    
    sf::RenderTexture renderTexture_;
    sf::Sprite renderSprite_;
    
    // Предзагруженные изображения текстур для производительности
    std::unordered_map<int, sf::Image> wallTextureImages_;
    std::unordered_map<int, sf::Image> floorTextureImages_;
    std::unordered_map<int, sf::Image> ceilingTextureImages_;
    
    // Карта текстур для стен (старая версия для совместимости)
    std::unordered_map<int, const sf::Texture*> wallTextures_;
    
    void updateRenderSpriteScale();
    
    bool useTextures_ = true;
    
    // Высота точки обзора (0.0 = пол, 1.0 = потолок)
    float viewHeight_ = 0.66f;
    
    // Максимальное расстояние затемнения
    float maxDarkDistance_ = 5.0f;
    
    // Старые имена для совместимости
    std::string floorTextureName_;
    std::string ceilingTextureName_;
    std::vector<std::string> wallTextureNames_;
};