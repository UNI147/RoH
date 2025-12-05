#pragma once
#include <SFML/Graphics.hpp>
#include <unordered_map>
#include <vector>
#include <string>
#include "../raycasting/RayCaster.h"
#include "../geographer/Furniture.h"
#include <iostream>
#include "resource_manager/ResourceManager.h"

class Renderer {
public:
    Renderer(sf::RenderWindow& window);
    
    // Основной метод рендеринга с тремя текстурными слоями
    void renderFrame(const PlayerState& player, 
                     const std::vector<std::vector<int>>& wallMap,
                     const std::vector<std::vector<int>>& floorMap,
                     const std::vector<std::vector<int>>& ceilingMap,
                     RayCaster& rayCaster);
    
    // Управление текстурами
    void addWallTexture(int textureId, const std::string& textureName);
    void addFloorTexture(int textureId, const std::string& textureName);
    void addCeilingTexture(int textureId, const std::string& textureName);
    
    // Очистка текстур
    void clearTextures();
    
    // Проверка наличия текстур
    bool hasTextures() const;
    size_t getWallTextureCount() const;
    size_t getFloorTextureCount() const;
    size_t getCeilingTextureCount() const;
    
    // Управление высотой камеры
    void setViewHeight(float height);
    float getViewHeight() const;
    
    // Управление режимом текстур
    void setUseTextures(bool use);
    
    // Старые методы для совместимости
    void setWallTexture(int wallType, const std::string& textureName);
    void setFloorTexture(const std::string& textureName);
    void setCeilingTexture(const std::string& textureName);
    void addWallTexture(const std::string& textureName);

    // Рендеринг мебели
    void renderFurniture(const PlayerState& player,
                        const std::vector<FurnitureObject>& furniture,
                        const std::unordered_map<std::string, FurnitureData>& furnitureTypes,
                        RayCaster& rayCaster);
    
    // Добавление текстур мебели
    void addFurnitureTexture(const std::string& furnitureName, const std::string& textureName);

    // Установка коэффициентов освещения
    void setLightBalance(float wallFactor, float floorFactor, float ceilingFactor) {
        wallLightFactor_ = wallFactor;
        floorLightFactor_ = floorFactor;
        ceilingLightFactor_ = ceilingFactor;
    }

    void setLightFactors(float wallFactor, float floorFactor, float ceilingFactor, float furnitureFactor = 1.0f) {
        wallLightFactor_ = wallFactor;
        floorLightFactor_ = floorFactor;
        ceilingLightFactor_ = ceilingFactor;
        furnitureLightFactor_ = furnitureFactor;
    }

    // Управление максимальным расстоянием затемнения
    void setMaxDarkDistance(float distance) { maxDarkDistance_ = distance; }
    float getMaxDarkDistance() const { return maxDarkDistance_; }
    
    void collectLightSourcesForScene(const std::vector<FurnitureObject>& furniture,
                                   const std::unordered_map<std::string, FurnitureData>& furnitureTypes);

    void display();

private:
    sf::RenderWindow& window_;
    
    void drawWallStrip(int x, int drawStart, int drawEnd, int side, float distance);
    void drawFloorAndCeiling(int x, int drawStart, int drawEnd);
    void drawTexturedWallStrip(int x, int drawStart, int drawEnd, 
                              const RayHit& hit, int lineHeight);
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
    
    // Текстуры мебели
    std::unordered_map<std::string, sf::Image> furnitureTextureImages_;
    
    // Вспомогательные методы
    void drawFurnitureSprite(const FurnitureObject& obj, 
                           const FurnitureData& data,
                           const PlayerState& player,
                           float distance,
                           float angleToPlayer);
                           
    void renderFurnitureInternal(const PlayerState& player,
                                const std::vector<FurnitureObject>& furniture,
                                const std::unordered_map<std::string, FurnitureData>& furnitureTypes);
    
    // Новые методы с Z-буфером (переименованы для избежания конфликтов)
    void drawFurnitureSpriteWithDepth(const FurnitureObject& obj, 
                                   const FurnitureData& data,
                                   const PlayerState& player,
                                   float distance,
                                   float playerAngle,
                                   const std::vector<float>& depthBuffer);
    
    void renderFurnitureWithDepth(const PlayerState& player,
                                const std::vector<FurnitureObject>& furniture,
                                const std::unordered_map<std::string, FurnitureData>& furnitureTypes,
                                const std::vector<float>& depthBuffer);
                                
    const std::vector<std::vector<int>>* currentWallMap_ = nullptr;
    
    // Светящиеся объекты для расчета освещения
    struct LightSource {
        sf::Vector2f position;
        float radius;
        float intensity;
    };
    std::vector<LightSource> lightSources_;
    
    // Метод для сбора источников света
    void collectLightSources(const std::vector<FurnitureObject>& furniture,
                           const std::unordered_map<std::string, FurnitureData>& furnitureTypes);
    
    // Метод для проверки видимости света
    bool isLightVisible(const sf::Vector2f& lightPos, const sf::Vector2f& targetPos,
                       const std::vector<std::vector<int>>& wallMap) const;
    
    bool isPointInSameCell(float worldX, float worldY, int cellX, int cellY) const;

    float wallLightFactor_ = 0.75f;
    float floorLightFactor_ = 0.75f;
    float ceilingLightFactor_ = 0.75f;
    float furnitureLightFactor_ = 0.75f;

    float calculateLightAtPoint(const sf::Vector2f& point, 
                            const std::vector<std::vector<int>>& wallMap) const;
};