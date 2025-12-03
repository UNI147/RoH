#include "Renderer.h"
#include <cmath>
#include <iostream>
#include <algorithm>
#include "resource_manager/ResourceManager.h"

Renderer::Renderer(sf::RenderWindow& window) 
    : window_(window) {
    
    if (renderTexture_.create(RENDER_WIDTH, RENDER_HEIGHT)) {
        renderTexture_.setSmooth(false);
        renderSprite_.setTexture(renderTexture_.getTexture());
        updateRenderSpriteScale();
        renderSprite_.setTextureRect(sf::IntRect(0, 0, RENDER_WIDTH, RENDER_HEIGHT));
    }
    
    // Начальное состояние - текстуры не загружены
    useTextures_ = false;
    
    std::cout << "Renderer created (textures disabled by default)" << std::endl;
}

// Методы для управления высотой обзора
void Renderer::setViewHeight(float height) {
    viewHeight_ = std::max(0.0f, std::min(1.0f, height));
}

float Renderer::getViewHeight() const {
    return viewHeight_;
}

void Renderer::renderFrame(const PlayerState& player, 
                          const std::vector<std::vector<int>>& wallMap,
                          const std::vector<std::vector<int>>& floorMap,
                          const std::vector<std::vector<int>>& ceilingMap,
                          RayCaster& rayCaster) {
    updateRenderSpriteScale();
    renderTexture_.clear();
    
    // Проверяем, включены ли текстуры
    if (useTextures_ && hasTextures()) {
        drawTexturedFloorAndCeiling(player, floorMap, ceilingMap, rayCaster);
    } else {
        drawSolidFloorAndCeiling();
    }
    
    // Затем рендерим стены поверх
    for (int x = 0; x < RENDER_WIDTH; ++x) {
        float cameraX = 2 * x / float(RENDER_WIDTH) - 1;
        RayHit hit = rayCaster.castRay(player, wallMap, cameraX);
        
        // Правильный расчет высоты стены с учетом высоты обзора
        int lineHeight = static_cast<int>(RENDER_HEIGHT / hit.distance);
        
        // Расчет: viewHeight_ смещает точку обзора
        int horizonLine = static_cast<int>(RENDER_HEIGHT * viewHeight_);
        int drawStart = horizonLine - lineHeight / 2;
        int drawEnd = horizonLine + lineHeight / 2;
        
        if (drawStart < 0) drawStart = 0;
        if (drawEnd >= RENDER_HEIGHT) drawEnd = RENDER_HEIGHT - 1;
        
        if (useTextures_ && hasTextures()) {
            drawTexturedWallStrip(x, drawStart, drawEnd, hit, lineHeight);
        } else {
            drawWallStrip(x, drawStart, drawEnd, hit.side, hit.distance);
        }
    }
    
    renderTexture_.display();
    window_.clear(sf::Color::Black);
    window_.draw(renderSprite_);
}

void Renderer::drawTexturedWallStrip(int x, int drawStart, int drawEnd, 
                                    const RayHit& hit, int lineHeight) {
    if (lineHeight <= 0) return;
    
    // Используем ID текстуры из hit
    int texId = hit.textureId;
    
    // Находим соответствующее изображение текстуры по ID
    sf::Image* textureImage = nullptr;
    auto it = wallTextureImages_.find(texId);
    if (it != wallTextureImages_.end()) {
        textureImage = &it->second;
    } else if (!wallTextureImages_.empty()) {
        // Fallback: ищем любую текстуру стен
        textureImage = &wallTextureImages_.begin()->second;
        std::cout << "Warning: Wall texture ID " << texId << " not found, using fallback" << std::endl;
    } else {
        return;
    }
    
    // Вычисление координаты текстуры по X
    int texWidth = static_cast<int>(textureImage->getSize().x);
    int texX = static_cast<int>(hit.wallX * static_cast<float>(texWidth));
    
    // Корректировка для предотвращения зеркального отображения
    if ((hit.side == 0 && hit.rayDirX > 0) || (hit.side == 1 && hit.rayDirY < 0)) {
        texX = textureImage->getSize().x - texX - 1;
    }
    
    // Обеспечиваем, чтобы texX был в пределах текстуры
    texX = std::max(0, std::min(texX, static_cast<int>(textureImage->getSize().x) - 1));
    
    // Вычисление шага текстуры по Y
    float step = static_cast<float>(textureImage->getSize().y) / static_cast<float>(lineHeight);
    
    // Расчет позиции текстуры с учетом высоты обзора
    float texPos = (static_cast<float>(drawStart) - static_cast<float>(RENDER_HEIGHT) * viewHeight_ + static_cast<float>(lineHeight) / 2.0f) * step;
    
    // Интенсивное затенение
    float brightness = 1.0f - (hit.distance / maxDarkDistance_);
    brightness = std::max(0.0f, std::min(1.0f, brightness));
    
    // Дополнительное затемнение для Y-сторон
    if (hit.side == 1) {
        brightness *= 0.7f;
    }
    
    for (int y = drawStart; y < drawEnd; ++y) {
        int texY = static_cast<int>(texPos) % textureImage->getSize().y;
        texY = std::max(0, std::min(texY, static_cast<int>(textureImage->getSize().y) - 1));
        
        sf::Color pixelColor = textureImage->getPixel(texX, texY);
        
        // Применяем интенсивное затемнение
        pixelColor.r = static_cast<sf::Uint8>(static_cast<float>(pixelColor.r) * brightness);
        pixelColor.g = static_cast<sf::Uint8>(static_cast<float>(pixelColor.g) * brightness);
        pixelColor.b = static_cast<sf::Uint8>(static_cast<float>(pixelColor.b) * brightness);
        
        // Рисуем один пиксель
        sf::Vertex pixel(sf::Vector2f(static_cast<float>(x), static_cast<float>(y)), pixelColor);
        renderTexture_.draw(&pixel, 1, sf::Points);
        
        texPos += step;
    }
}

void Renderer::drawTexturedFloorAndCeiling(const PlayerState& player, 
                                          const std::vector<std::vector<int>>& floorMap,
                                          const std::vector<std::vector<int>>& ceilingMap,
                                          RayCaster& rayCaster) {
    // Исправленный рендеринг пола и потолка с учетом высоты обзора
    int horizonLine = static_cast<int>(RENDER_HEIGHT * viewHeight_);
    
    // Рендерим пол (ниже горизонта)
    for (int y = horizonLine + 1; y < RENDER_HEIGHT; ++y) {
        // Луч для пола
        float rayDirX0 = player.direction.x - player.plane.x;
        float rayDirY0 = player.direction.y - player.plane.y;
        float rayDirX1 = player.direction.x + player.plane.x;
        float rayDirY1 = player.direction.y + player.plane.y;
        
        // Текущая позиция ряда пикселей относительно горизонта
        float p = static_cast<float>(y - horizonLine);
        
        // Вертикальная позиция камеры (учитываем высоту обзора)
        float posZ = 0.5f * static_cast<float>(RENDER_HEIGHT);
        
        // Горизонтальное расстояние от камеры к полу для текущего ряда
        float rowDistance = posZ / p;
        
        // Вычисляем реальные шаги
        float floorStepX = rowDistance * (rayDirX1 - rayDirX0) / static_cast<float>(RENDER_WIDTH);
        float floorStepY = rowDistance * (rayDirY1 - rayDirY0) / static_cast<float>(RENDER_WIDTH);
        
        // Реальная координата пола
        float floorX = player.position.x + rowDistance * rayDirX0;
        float floorY = player.position.y + rowDistance * rayDirY0;
        
        for (int x = 0; x < RENDER_WIDTH; ++x) {
            // Координаты в сетке
            int cellX = static_cast<int>(floorX);
            int cellY = static_cast<int>(floorY);
            
            if (cellY >= 0 && cellY < static_cast<int>(floorMap.size()) &&
                cellX >= 0 && cellX < static_cast<int>(floorMap[0].size())) {
                
                // Получаем ID текстуры пола
                int texId = floorMap[cellY][cellX];
                
                // Определяем изображение текстуры пола
                sf::Image* floorTexImage = nullptr;
                auto it = floorTextureImages_.find(texId);
                if (it != floorTextureImages_.end()) {
                    floorTexImage = &it->second;
                } else if (!floorTextureImages_.empty()) {
                    // Fallback на первую текстуру
                    floorTexImage = &floorTextureImages_.begin()->second;
                }
                
                if (floorTexImage) {
                    // Координаты текстуры для пола
                    int floorTexX = static_cast<int>(static_cast<float>(floorTexImage->getSize().x) * (floorX - std::floor(floorX)));
                    int floorTexY = static_cast<int>(static_cast<float>(floorTexImage->getSize().y) * (floorY - std::floor(floorY)));
                    
                    // Ограничиваем координаты текстуры
                    floorTexX = std::max(0, std::min(floorTexX, static_cast<int>(floorTexImage->getSize().x) - 1));
                    floorTexY = std::max(0, std::min(floorTexY, static_cast<int>(floorTexImage->getSize().y) - 1));
                    
                    // Интенсивное затенение для пола
                    float floorBrightness = 1.0f - (rowDistance / maxDarkDistance_);
                    floorBrightness = std::max(0.0f, std::min(1.0f, floorBrightness));
                    
                    // Цвет пола
                    sf::Color floorColor = floorTexImage->getPixel(floorTexX, floorTexY);
                    floorColor.r = static_cast<sf::Uint8>(static_cast<float>(floorColor.r) * floorBrightness);
                    floorColor.g = static_cast<sf::Uint8>(static_cast<float>(floorColor.g) * floorBrightness);
                    floorColor.b = static_cast<sf::Uint8>(static_cast<float>(floorColor.b) * floorBrightness);
                    
                    // Отрисовка пикселя пола
                    sf::Vertex floorPixel(sf::Vector2f(static_cast<float>(x), static_cast<float>(y)), floorColor);
                    renderTexture_.draw(&floorPixel, 1, sf::Points);
                }
            }
            
            floorX += floorStepX;
            floorY += floorStepY;
        }
    }
    
    // Рендерим потолок (выше горизонта)
    for (int y = 0; y < horizonLine; ++y) {
        // Луч для потолка
        float rayDirX0 = player.direction.x - player.plane.x;
        float rayDirY0 = player.direction.y - player.plane.y;
        float rayDirX1 = player.direction.x + player.plane.x;
        float rayDirY1 = player.direction.y + player.plane.y;
        
        // Текущая позиция ряда пикселей относительно горизонта
        float p = static_cast<float>(horizonLine - y);
        
        // Вертикальная позиция камеры
        float posZ = 0.5f * static_cast<float>(RENDER_HEIGHT);
        
        // Горизонтальное расстояние от камеры к потолку для текущего ряда
        float rowDistance = posZ / p;
        
        // Вычисляем реальные шаги
        float ceilingStepX = rowDistance * (rayDirX1 - rayDirX0) / static_cast<float>(RENDER_WIDTH);
        float ceilingStepY = rowDistance * (rayDirY1 - rayDirY0) / static_cast<float>(RENDER_WIDTH);
        
        // Реальная координата потолка
        float ceilingX = player.position.x + rowDistance * rayDirX0;
        float ceilingY = player.position.y + rowDistance * rayDirY0;
        
        for (int x = 0; x < RENDER_WIDTH; ++x) {
            // Координаты в сетке
            int cellX = static_cast<int>(ceilingX);
            int cellY = static_cast<int>(ceilingY);
            
            if (cellY >= 0 && cellY < static_cast<int>(ceilingMap.size()) &&
                cellX >= 0 && cellX < static_cast<int>(ceilingMap[0].size())) {
                
                // Получаем ID текстуры потолка
                int texId = ceilingMap[cellY][cellX];
                
                // Определяем изображение текстуры потолка
                sf::Image* ceilingTexImage = nullptr;
                auto it = ceilingTextureImages_.find(texId);
                if (it != ceilingTextureImages_.end()) {
                    ceilingTexImage = &it->second;
                } else if (!ceilingTextureImages_.empty()) {
                    // Fallback на первую текстуру
                    ceilingTexImage = &ceilingTextureImages_.begin()->second;
                }
                
                if (ceilingTexImage) {
                    // Координаты текстуры для потолка
                    int ceilingTexX = static_cast<int>(static_cast<float>(ceilingTexImage->getSize().x) * (ceilingX - std::floor(ceilingX)));
                    int ceilingTexY = static_cast<int>(static_cast<float>(ceilingTexImage->getSize().y) * (ceilingY - std::floor(ceilingY)));
                    
                    // Ограничиваем координаты текстуры
                    ceilingTexX = std::max(0, std::min(ceilingTexX, static_cast<int>(ceilingTexImage->getSize().x) - 1));
                    ceilingTexY = std::max(0, std::min(ceilingTexY, static_cast<int>(ceilingTexImage->getSize().y) - 1));
                    
                    // Интенсивное затенение для потолка
                    float ceilingBrightness = 1.0f - (rowDistance / maxDarkDistance_);
                    ceilingBrightness = std::max(0.0f, std::min(1.0f, ceilingBrightness));
                    
                    sf::Color ceilingColor = ceilingTexImage->getPixel(ceilingTexX, ceilingTexY);
                    ceilingColor.r = static_cast<sf::Uint8>(static_cast<float>(ceilingColor.r) * ceilingBrightness);
                    ceilingColor.g = static_cast<sf::Uint8>(static_cast<float>(ceilingColor.g) * ceilingBrightness);
                    ceilingColor.b = static_cast<sf::Uint8>(static_cast<float>(ceilingColor.b) * ceilingBrightness);
                    
                    sf::Vertex ceilingPixel(sf::Vector2f(static_cast<float>(x), static_cast<float>(y)), ceilingColor);
                    renderTexture_.draw(&ceilingPixel, 1, sf::Points);
                }
            }
            
            ceilingX += ceilingStepX;
            ceilingY += ceilingStepY;
        }
    }
}

void Renderer::drawSolidFloorAndCeiling() {
    // Исправленная заливка пола и потолка с учетом высоты обзора
    int horizonLine = static_cast<int>(RENDER_HEIGHT * viewHeight_);
    
    // Рисуем пол (ниже горизонта)
    for (int y = horizonLine; y < RENDER_HEIGHT; ++y) {
        for (int x = 0; x < RENDER_WIDTH; ++x) {
            sf::Vertex floorPixel(sf::Vector2f(static_cast<float>(x), static_cast<float>(y)), sf::Color(50, 50, 50));
            renderTexture_.draw(&floorPixel, 1, sf::Points);
        }
    }
    
    // Рисуем потолок (выше горизонта)
    for (int y = 0; y < horizonLine; ++y) {
        for (int x = 0; x < RENDER_WIDTH; ++x) {
            sf::Vertex ceilingPixel(sf::Vector2f(static_cast<float>(x), static_cast<float>(y)), sf::Color(100, 100, 100));
            renderTexture_.draw(&ceilingPixel, 1, sf::Points);
        }
    }
}

void Renderer::updateRenderSpriteScale() {
    sf::Vector2u windowSize = window_.getSize();
    float scaleX = static_cast<float>(windowSize.x) / static_cast<float>(RENDER_WIDTH);
    float scaleY = static_cast<float>(windowSize.y) / static_cast<float>(RENDER_HEIGHT);
    float scale = std::min(scaleX, scaleY);
    
    renderSprite_.setScale(scale, scale);
    
    float offsetX = (static_cast<float>(windowSize.x) - static_cast<float>(RENDER_WIDTH) * scale) / 2.0f;
    float offsetY = (static_cast<float>(windowSize.y) - static_cast<float>(RENDER_HEIGHT) * scale) / 2.0f;
    renderSprite_.setPosition(offsetX, offsetY);
}

void Renderer::drawWallStrip(int x, int drawStart, int drawEnd, int side, float distance) {
    sf::Color wallColor;
    if (side == 0) {
        wallColor = sf::Color(200, 100, 100);
    } else {
        wallColor = sf::Color(100, 100, 200);
    }
    
    // Интенсивное затенение для цветных стен
    float brightness = 1.0f - (distance / maxDarkDistance_);
    brightness = std::max(0.0f, std::min(1.0f, brightness));
    
    wallColor.r = static_cast<sf::Uint8>(static_cast<float>(wallColor.r) * brightness);
    wallColor.g = static_cast<sf::Uint8>(static_cast<float>(wallColor.g) * brightness);
    wallColor.b = static_cast<sf::Uint8>(static_cast<float>(wallColor.b) * brightness);
    
    sf::Vertex line[] = {
        sf::Vertex(sf::Vector2f(static_cast<float>(x), static_cast<float>(drawStart)), wallColor),
        sf::Vertex(sf::Vector2f(static_cast<float>(x), static_cast<float>(drawEnd)), wallColor)
    };
    
    renderTexture_.draw(line, 2, sf::Lines);
}

void Renderer::drawFloorAndCeiling(int x, int drawStart, int drawEnd) {
    // Пустая реализация для совместимости
}

void Renderer::setWallTexture(int wallType, const std::string& textureName) {
    auto& rm = ResourceManager::getInstance();
    if (rm.hasTexture(textureName)) {
        wallTextures_[wallType] = &rm.getTexture(textureName);
    }
}

void Renderer::setFloorTexture(const std::string& textureName) {
    floorTextureName_ = textureName;
    auto& rm = ResourceManager::getInstance();
    if (rm.hasTexture(textureName)) {
        // Добавляем как текстуру пола с ID 0
        floorTextureImages_[0] = rm.getTexture(textureName).copyToImage();
        std::cout << "Renderer: Floor texture set to " << textureName << std::endl;
    }
}

void Renderer::setCeilingTexture(const std::string& textureName) {
    ceilingTextureName_ = textureName;
    auto& rm = ResourceManager::getInstance();
    if (rm.hasTexture(textureName)) {
        // Добавляем как текстуру потолка с ID 0
        ceilingTextureImages_[0] = rm.getTexture(textureName).copyToImage();
        std::cout << "Renderer: Ceiling texture set to " << textureName << std::endl;
    }
}

void Renderer::addWallTexture(const std::string& textureName) {
    wallTextureNames_.push_back(textureName);
    auto& rm = ResourceManager::getInstance();
    if (rm.hasTexture(textureName)) {
        // Используем первую текстуру стен как основную с ID 0
        if (wallTextureNames_.size() == 1) {
            wallTextureImages_[0] = rm.getTexture(textureName).copyToImage();
            std::cout << "Renderer: Primary wall texture set to " << textureName << std::endl;
        }
    }
}

// Новые методы для управления текстурными слоями
void Renderer::addWallTexture(int textureId, const std::string& textureName) {
    auto& rm = ResourceManager::getInstance();
    if (rm.hasTexture(textureName)) {
        wallTextureImages_[textureId] = rm.getTexture(textureName).copyToImage();
        std::cout << "Renderer: Wall texture ID " << textureId << " set to " << textureName << std::endl;
    }
}

void Renderer::addFloorTexture(int textureId, const std::string& textureName) {
    auto& rm = ResourceManager::getInstance();
    if (rm.hasTexture(textureName)) {
        floorTextureImages_[textureId] = rm.getTexture(textureName).copyToImage();
        std::cout << "Renderer: Floor texture ID " << textureId << " set to " << textureName << std::endl;
    }
}

void Renderer::addCeilingTexture(int textureId, const std::string& textureName) {
    auto& rm = ResourceManager::getInstance();
    if (rm.hasTexture(textureName)) {
        ceilingTextureImages_[textureId] = rm.getTexture(textureName).copyToImage();
        std::cout << "Renderer: Ceiling texture ID " << textureId << " set to " << textureName << std::endl;
    }
}
    
void Renderer::clearTextures() {
    wallTextureImages_.clear();
    floorTextureImages_.clear();
    ceilingTextureImages_.clear();
    wallTextures_.clear();
    
    std::cout << "Renderer textures cleared" << std::endl;
}

bool Renderer::hasTextures() const {
    return !wallTextureImages_.empty() || !floorTextureImages_.empty() || !ceilingTextureImages_.empty();
}

size_t Renderer::getWallTextureCount() const {
    return wallTextureImages_.size();
}

size_t Renderer::getFloorTextureCount() const {
    return floorTextureImages_.size();
}

size_t Renderer::getCeilingTextureCount() const {
    return ceilingTextureImages_.size();
}

void Renderer::setUseTextures(bool use) {
    useTextures_ = use;
}