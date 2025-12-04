#include "Renderer.h"
#include <cmath>
#include <iostream>
#include <algorithm>
#include <limits>
#include "resource_manager/ResourceManager.h"
#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif
#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923f
#endif

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
    
    // Сохраняем текущую карту стен для Z-буфера мебели
    currentWallMap_ = &wallMap;
    
    updateRenderSpriteScale();
    renderTexture_.clear();
    
    // Проверяем, включены ли текстуры
    if (useTextures_ && hasTextures()) {
        drawTexturedFloorAndCeiling(player, floorMap, ceilingMap, rayCaster);
    } else {
        drawSolidFloorAndCeiling();
    }
    
    // Создаем буфер глубины для стен
    std::vector<float> wallDepthBuffer(RENDER_WIDTH);
    
    // Сначала рендерим стены и заполняем буфер глубины
    for (int x = 0; x < RENDER_WIDTH; ++x) {
        float cameraX = 2 * x / float(RENDER_WIDTH) - 1;
        RayHit hit = rayCaster.castRay(player, wallMap, cameraX);
        
        // Используем перпендикулярное расстояние для предотвращения искажений
        float perpWallDist = hit.distance;
        
        wallDepthBuffer[x] = perpWallDist;
        
        // Правильный расчет высоты стены с учетом высоты обзора
        int lineHeight = static_cast<int>(RENDER_HEIGHT / perpWallDist);
        
        // Расчет: viewHeight_ смещает точку обзора
        int horizonLine = static_cast<int>(RENDER_HEIGHT * viewHeight_);
        int drawStart = horizonLine - lineHeight / 2;
        int drawEnd = horizonLine + lineHeight / 2;
        
        if (drawStart < 0) drawStart = 0;
        if (drawEnd >= RENDER_HEIGHT) drawEnd = RENDER_HEIGHT - 1;
        
        if (useTextures_ && hasTextures()) {
            drawTexturedWallStrip(x, drawStart, drawEnd, hit, lineHeight);
        } else {
            drawWallStrip(x, drawStart, drawEnd, hit.side, perpWallDist);
        }
    }
}

// Метод для отображения всего:
void Renderer::display() {
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
    
    // Расстояние затемнение - уменьшаем максимальное расстояние для более сильного затемнения
    float distanceBrightness = 1.0f - std::min(1.0f, hit.distance / maxDarkDistance_);
    distanceBrightness = std::max(0.0f, distanceBrightness);
    
    // Дополнительное затемнение для Y-сторон
    float sideDarkness = 1.0f;
    if (hit.side == 1) {
        sideDarkness = 0.7f;
    }
    
    // Базовое затемнение
    float baseBrightness = distanceBrightness * sideDarkness;
    
    // Используем currentWallMap_ для проверки препятствий
    const std::vector<std::vector<int>>* wallMapPtr = currentWallMap_;
    
    for (int y = drawStart; y < drawEnd; ++y) {
        int texY = static_cast<int>(texPos) % textureImage->getSize().y;
        texY = std::max(0, std::min(texY, static_cast<int>(textureImage->getSize().y) - 1));
        
        sf::Color pixelColor = textureImage->getPixel(texX, texY);
        
        // Изначальная яркость - только расстояние и сторона
        float brightness = baseBrightness;
        
        // Добавляем освещение от источников света (с проверкой препятствий)
        if (!lightSources_.empty() && wallMapPtr) {
            // Позиция стены в мире
            float wallWorldX = hit.mapX + 0.5f; // Используем центр клетки для проверки
            float wallWorldY = hit.mapY + 0.5f;
            
            // Для Y-сторон корректируем позицию
            if (hit.side == 1) {
                if (hit.rayDirY > 0) wallWorldY -= 0.5f;
                else wallWorldY += 0.5f;
            } else {
                if (hit.rayDirX > 0) wallWorldX -= 0.5f;
                else wallWorldX += 0.5f;
            }
            
            for (const auto& light : lightSources_) {
                // Проверяем расстояние до центра клетки
                float dx = wallWorldX - light.position.x;
                float dy = wallWorldY - light.position.y;
                float lightDistSq = dx*dx + dy*dy;
                
                if (lightDistSq < light.radius * light.radius) {
                    float lightDist = std::sqrt(lightDistSq);
                    
                    // Проверяем несколько точек на стене
                    bool lightVisible = false;
                    
                    // Основная проверка (быстрая) - от центра клетки
                    if (isLightVisible(light.position, 
                                    sf::Vector2f(wallWorldX, wallWorldY), 
                                    *wallMapPtr)) {
                        lightVisible = true;
                    } else {
                        // Если свет не виден из центра, проверяем ближайший угол
                        float closestCornerX = (light.position.x < wallWorldX) ? 
                            wallWorldX - 0.4f : wallWorldX + 0.4f;
                        float closestCornerY = (light.position.y < wallWorldY) ? 
                            wallWorldY - 0.4f : wallWorldY + 0.4f;
                        
                        if (isLightVisible(light.position,
                                        sf::Vector2f(closestCornerX, closestCornerY),
                                        *wallMapPtr)) {
                            lightVisible = true;
                        }
                    }
                    
                    if (lightVisible) {
                        // Квадратичное затухание для более реалистичного света
                        float lightIntensity = 1.0f - (lightDist / light.radius);
                        lightIntensity = lightIntensity * lightIntensity;
                        lightIntensity *= light.intensity;
                        
                        // Для угловых стен уменьшаем интенсивность
                        if (!isPointInSameCell(wallWorldX, wallWorldY, 
                                            hit.mapX, hit.mapY)) {
                            lightIntensity *= 0.7f;
                        }
                        
                        // Ограничиваем максимальную яркость от источников света
                        lightIntensity = std::min(1.5f, lightIntensity);
                        
                        // Учитываем угол падения света (для реалистичности)
                        brightness = std::max(brightness, lightIntensity);
                    }
                }
            }
        }
        
        // Ограничиваем итоговую яркость - разрешаем полную темноту (0.0f)
        brightness = std::max(0.0f, std::min(2.0f, brightness));
        
        // Применяем яркость к цвету с защитой от переполнения
        float r = static_cast<float>(pixelColor.r) * brightness;
        float g = static_cast<float>(pixelColor.g) * brightness;
        float b = static_cast<float>(pixelColor.b) * brightness;
        
        pixelColor.r = static_cast<sf::Uint8>(std::min(255.0f, r));
        pixelColor.g = static_cast<sf::Uint8>(std::min(255.0f, g));
        pixelColor.b = static_cast<sf::Uint8>(std::min(255.0f, b));
        
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
    
    // Сохраняем карту стен для проверки освещения
    const std::vector<std::vector<int>>* wallMapPtr = currentWallMap_;
    if (!wallMapPtr) return;
    
    // Рендеринг пола и потолка с учетом высоты обзора
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
                    
                    // Базовое затенение пола - разрешаем полную темноту
                    float floorBrightness = 1.0f - std::min(1.0f, rowDistance / maxDarkDistance_);
                    floorBrightness = std::max(0.0f, floorBrightness);
                    
                    // ОСВЕЩЕНИЕ ДЛЯ ПОЛА С ПРОВЕРКОЙ ПРЕПЯТСТВИЙ
                    float floorLightBrightness = 0.0f;
                    if (!lightSources_.empty() && wallMapPtr) {
                        float floorWorldX = floorX;
                        float floorWorldY = floorY;
                        
                        for (const auto& light : lightSources_) {
                            float lightDist = std::sqrt(
                                (floorWorldX - light.position.x) * (floorWorldX - light.position.x) +
                                (floorWorldY - light.position.y) * (floorWorldY - light.position.y)
                            );
                            
                            if (lightDist < light.radius) {
                                // ПРОВЕРЯЕМ, НЕ БЛОКИРОВАН ЛИ СВЕТ
                                if (isLightVisible(light.position, 
                                                   sf::Vector2f(floorWorldX, floorWorldY), 
                                                   *wallMapPtr)) {
                                    
                                    // Квадратичное затухание
                                    float lightIntensity = 1.0f - (lightDist / light.radius);
                                    lightIntensity = lightIntensity * lightIntensity;
                                    lightIntensity *= light.intensity;
                                    floorLightBrightness = std::max(floorLightBrightness, lightIntensity);
                                }
                            }
                        }
                    }
                    
                    // Итоговая яркость пола - СУММИРУЕМ затемнение и освещение
                    float finalFloorBrightness = floorBrightness + floorLightBrightness;
                    finalFloorBrightness = std::max(0.0f, std::min(2.0f, finalFloorBrightness));
                    
                    // Цвет пола с защитой от переполнения
                    sf::Color floorColor = floorTexImage->getPixel(floorTexX, floorTexY);
                    float r = static_cast<float>(floorColor.r) * finalFloorBrightness;
                    float g = static_cast<float>(floorColor.g) * finalFloorBrightness;
                    float b = static_cast<float>(floorColor.b) * finalFloorBrightness;
                    
                    floorColor.r = static_cast<sf::Uint8>(std::min(255.0f, r));
                    floorColor.g = static_cast<sf::Uint8>(std::min(255.0f, g));
                    floorColor.b = static_cast<sf::Uint8>(std::min(255.0f, b));
                    
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
                    
                    // Базовое затенение для потолка - разрешаем полную темноту
                    float ceilingBrightness = 1.0f - std::min(1.0f, rowDistance / maxDarkDistance_);
                    ceilingBrightness = std::max(0.0f, ceilingBrightness);
                    
                    // ОСВЕЩЕНИЕ ДЛЯ ПОТОЛКА С ПРОВЕРКОЙ ПРЕПЯТСТВИЙ
                    float ceilingLightBrightness = 0.0f;
                    if (!lightSources_.empty() && wallMapPtr) {
                        float ceilingWorldX = ceilingX;
                        float ceilingWorldY = ceilingY;
                        
                        for (const auto& light : lightSources_) {
                            float lightDist = std::sqrt(
                                (ceilingWorldX - light.position.x) * (ceilingWorldX - light.position.x) +
                                (ceilingWorldY - light.position.y) * (ceilingWorldY - light.position.y)
                            );
                            
                            if (lightDist < light.radius) {
                                // ПРОВЕРЯЕМ, НЕ БЛОКИРОВАН ЛИ СВЕТ
                                if (isLightVisible(light.position, 
                                                   sf::Vector2f(ceilingWorldX, ceilingWorldY), 
                                                   *wallMapPtr)) {
                                    
                                    // Квадратичное затухание
                                    float lightIntensity = 1.0f - (lightDist / light.radius);
                                    lightIntensity = lightIntensity * lightIntensity;
                                    lightIntensity *= light.intensity;
                                    ceilingLightBrightness = std::max(ceilingLightBrightness, lightIntensity);
                                }
                            }
                        }
                    }
                    
                    // Итоговая яркость потолка
                    float finalCeilingBrightness = ceilingBrightness + ceilingLightBrightness;
                    finalCeilingBrightness = std::max(0.0f, std::min(2.0f, finalCeilingBrightness));
                    
                    sf::Color ceilingColor = ceilingTexImage->getPixel(ceilingTexX, ceilingTexY);
                    float r = static_cast<float>(ceilingColor.r) * finalCeilingBrightness;
                    float g = static_cast<float>(ceilingColor.g) * finalCeilingBrightness;
                    float b = static_cast<float>(ceilingColor.b) * finalCeilingBrightness;
                    
                    ceilingColor.r = static_cast<sf::Uint8>(std::min(255.0f, r));
                    ceilingColor.g = static_cast<sf::Uint8>(std::min(255.0f, g));
                    ceilingColor.b = static_cast<sf::Uint8>(std::min(255.0f, b));
                    
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


void Renderer::renderFurniture(const PlayerState& player,
                              const std::vector<FurnitureObject>& furniture,
                              const std::unordered_map<std::string, FurnitureData>& furnitureTypes,
                              RayCaster& rayCaster) {
    
    if (!useTextures_ || furniture.empty() || furnitureTextureImages_.empty()) {
        return;
    }
    
    // Проверяем, есть ли текущая карта стен
    if (!currentWallMap_) {
        std::cout << "Renderer: Warning - no current wall map for furniture rendering" << std::endl;
        return;
    }
    
    // Создаем буфер глубины для правильного рендеринга
    std::vector<float> depthBuffer(RENDER_WIDTH, std::numeric_limits<float>::max());
    
    // Заполняем буфер глубины стенами (разыменовываем указатель)
    for (int x = 0; x < RENDER_WIDTH; ++x) {
        float cameraX = 2 * x / float(RENDER_WIDTH) - 1;
        RayHit hit = rayCaster.castRay(player, *currentWallMap_, cameraX);
        depthBuffer[x] = hit.distance;
    }
    
    // Теперь рендерим мебель с учетом буфера глубины
    renderFurnitureWithDepth(player, furniture, furnitureTypes, depthBuffer);
}

void Renderer::collectLightSources(const std::vector<FurnitureObject>& furniture,
                                 const std::unordered_map<std::string, FurnitureData>& furnitureTypes) {
    lightSources_.clear();
    
    for (const auto& obj : furniture) {
        auto it = furnitureTypes.find(obj.furnitureType);
        if (it != furnitureTypes.end() && it->second.lightRadius > 0.0f) {
            LightSource light;
            light.position = obj.position;
            light.radius = it->second.lightRadius;
            light.intensity = 1.0f;
            lightSources_.push_back(light);
        }
    }
}

void Renderer::renderFurnitureWithDepth(const PlayerState& player,
                                      const std::vector<FurnitureObject>& furniture,
                                      const std::unordered_map<std::string, FurnitureData>& furnitureTypes,
                                      const std::vector<float>& depthBuffer) {
    
    if (!useTextures_ || furniture.empty() || furnitureTextureImages_.empty()) {
        return;
    }
    
    // Собираем источники света
    collectLightSources(furniture, furnitureTypes);
    
    std::vector<std::pair<float, const FurnitureObject*>> sortedFurniture;
    
    for (const auto& obj : furniture) {
        float dx = obj.position.x - player.position.x;
        float dy = obj.position.y - player.position.y;
        float distance = std::sqrt(dx*dx + dy*dy);
        
        // Отбрасываем слишком далекие объекты
        if (distance > 20.0f) continue;
        
        // Проверяем, есть ли тип мебели
        if (furnitureTypes.find(obj.furnitureType) == furnitureTypes.end()) {
            continue;
        }
        
        // Проверяем, находится ли объект перед камерой
        float dot = dx * player.direction.x + dy * player.direction.y;
        if (dot <= 0) continue;
        
        sortedFurniture.emplace_back(distance, &obj);
    }
    
    if (sortedFurniture.empty()) {
        return;
    }
    
    // Сортировка от дальних к ближним (painter's algorithm)
    std::sort(sortedFurniture.begin(), sortedFurniture.end(),
              [](const auto& a, const auto& b) { return a.first > b.first; });
    
    // Рендерим каждый объект
    for (const auto& [distance, objPtr] : sortedFurniture) {
        const auto& obj = *objPtr;
        const auto& data = furnitureTypes.at(obj.furnitureType);
        
        drawFurnitureSpriteWithDepth(obj, data, player, distance, 0.0f, depthBuffer);
    }
}

void Renderer::addFurnitureTexture(const std::string& furnitureName, const std::string& textureName) {
    auto& rm = ResourceManager::getInstance();
    if (rm.hasTexture(textureName)) {
        furnitureTextureImages_[furnitureName] = rm.getTexture(textureName).copyToImage();
        std::cout << "Renderer: Furniture texture " << furnitureName 
                << " set to " << textureName << std::endl;
    }
}

// Метод с Z-буфером
void Renderer::drawFurnitureSpriteWithDepth(const FurnitureObject& obj, 
                                   const FurnitureData& data,
                                   const PlayerState& player,
                                   float distance,
                                   float playerAngle,
                                   const std::vector<float>& depthBuffer) {
    
    if (distance < 0.1f) return;
    
    // Получаем текстуру
    auto textureIt = furnitureTextureImages_.find(obj.furnitureType);
    if (textureIt == furnitureTextureImages_.end()) {
        return;
    }
    
    const sf::Image& textureImage = textureIt->second;
    
    // 1. Вычисляем вектор от игрока к объекту
    float dx = obj.position.x - player.position.x;
    float dy = obj.position.y - player.position.y;
    
    // 2. Вычисляем трансформированную позицию объекта относительно направления камеры
    float invDet = 1.0f / (player.plane.x * player.direction.y - player.direction.x * player.plane.y);
    
    // Вектор от камеры к объекту
    float transformX = invDet * (player.direction.y * dx - player.direction.x * dy);
    float transformY = invDet * (-player.plane.y * dx + player.plane.x * dy);
    
    if (transformY <= 0) return;
    
    // 3. Вычисляем экранную координату X
    int spriteScreenX = static_cast<int>((RENDER_WIDTH / 2) * (1 + transformX / transformY));
    
    // 4. Рассчитываем высоту и ширину спрайта на экране
    int spriteHeight = std::abs(static_cast<int>(RENDER_HEIGHT / transformY));
    int spriteWidth = spriteHeight * data.width / data.height;
    
    // Если размер слишком мал, не рисуем
    if (spriteHeight < 1 || spriteWidth < 1) return;
    
    // 5. Вычисляем границы отрисовки по X
    int drawStartX = spriteScreenX - spriteWidth / 2;
    int drawEndX = drawStartX + spriteWidth;
    
    // Если спрайт полностью за пределами экрана, пропускаем
    if (drawStartX >= RENDER_WIDTH || drawEndX <= 0) return;
    
    // 6. Вычисляем границы отрисовки по Y
    int horizonLine = static_cast<int>(RENDER_HEIGHT * viewHeight_);
    int drawStartY = horizonLine - spriteHeight / 2;
    int drawEndY = drawStartY + spriteHeight;
    
    // Применяем вертикальное смещение (yOffset)
    int yOffset = static_cast<int>(data.yOffset * spriteHeight);
    drawStartY += yOffset;
    drawEndY += yOffset;
    
    // Если спрайт полностью за пределами экрана по Y, пропускаем
    if (drawStartY >= RENDER_HEIGHT || drawEndY <= 0) return;
    
    // 7. Определяем фактическую область отрисовки (клиппинг)
    int clipStartX = std::max(0, drawStartX);
    int clipEndX = std::min(RENDER_WIDTH, drawEndX);
    int clipStartY = std::max(0, drawStartY);
    int clipEndY = std::min(RENDER_HEIGHT, drawEndY);
    
    // 8. Вычисляем начальные координаты текстуры
    int texWidth = textureImage.getSize().x;
    int texHeight = textureImage.getSize().y;
    
    float stepX = static_cast<float>(texWidth) / static_cast<float>(spriteWidth);
    float stepY = static_cast<float>(texHeight) / static_cast<float>(spriteHeight);
    
    // Корректируем начальную текстуру для усеченного спрайта
    float texXOffset = (clipStartX - drawStartX) * stepX;
    float texYOffset = (clipStartY - drawStartY) * stepY;
    
    // Получаем карту стен
    const std::vector<std::vector<int>>* wallMapPtr = currentWallMap_;
    
    // 9. Затемнение в зависимости от расстояния - разрешаем полную темноту
    float distanceBrightness = 1.0f - std::min(1.0f, distance / maxDarkDistance_);
    distanceBrightness = std::max(0.0f, distanceBrightness);
    
    // 9а. Расчет освещения от источников света С ПРОВЕРКОЙ ПРЕПЯТСТВИЙ
    float lightBrightness = 0.0f;
    for (const auto& light : lightSources_) {
        // Расстояние от объекта до источника света
        float lightDist = std::sqrt(
            (obj.position.x - light.position.x) * (obj.position.x - light.position.x) +
            (obj.position.y - light.position.y) * (obj.position.y - light.position.y)
        );
        
        if (lightDist < light.radius) {
            // ПРОВЕРЯЕМ, НЕ БЛОКИРОВАН ЛИ СВЕТ СТЕНОЙ
            bool lightVisible = true;
            if (wallMapPtr) {
                lightVisible = isLightVisible(light.position, obj.position, *wallMapPtr);
            }
            
            if (lightVisible) {
                // Квадратичное затухание
                float lightIntensity = 1.0f - (lightDist / light.radius);
                lightIntensity = lightIntensity * lightIntensity;
                lightIntensity *= light.intensity;
                lightBrightness = std::max(lightBrightness, lightIntensity);
            }
        }
    }
    
    // 9б. Итоговая яркость: затемнение + освещение
    float brightness = distanceBrightness + lightBrightness;
    brightness = std::max(0.0f, std::min(2.0f, brightness));
    
    // Для светящихся объектов увеличиваем минимальную яркость
    if (data.lightRadius > 0.0f) {
        // Но не заставляем их светиться, если они должны быть темными
        brightness = std::max(brightness, 0.3f);
    }
    
    // 10. Подготавливаем вращение объекта
    bool useRotation = std::abs(obj.rotation) > 0.01f;
    float rad = obj.rotation * M_PI / 180.0f;
    float cosA = std::cos(rad);
    float sinA = std::sin(rad);
    float centerX = texWidth / 2.0f;
    float centerY = texHeight / 2.0f;
    
    // 11. Рисуем пиксели с проверкой Z-буфера
    for (int y = clipStartY; y < clipEndY; ++y) {
        float texY = texYOffset + (y - clipStartY) * stepY;
        
        for (int x = clipStartX; x < clipEndX; ++x) {
            // Пропускаем если стена ближе
            if (depthBuffer[x] < transformY - 0.1f) continue;
            
            float texX = texXOffset + (x - clipStartX) * stepX;
            
            int texXInt, texYInt;
            
            // Применяем вращение если нужно
            if (useRotation) {
                float relX = texX - centerX;
                float relY = texY - centerY;
                
                float rotatedX = relX * cosA - relY * sinA + centerX;
                float rotatedY = relX * sinA + relY * cosA + centerY;
                
                // Проверяем границы после вращения
                if (rotatedX >= 0 && rotatedX < texWidth &&
                    rotatedY >= 0 && rotatedY < texHeight) {
                    texXInt = static_cast<int>(rotatedX);
                    texYInt = static_cast<int>(rotatedY);
                } else {
                    continue;
                }
            } else {
                texXInt = static_cast<int>(texX);
                texYInt = static_cast<int>(texY);
            }
            
            sf::Color pixelColor = textureImage.getPixel(texXInt, texYInt);
            
            // Пропускаем прозрачные пиксели
            if (pixelColor.a < 10) continue;
            
            // Применяем затемнение с защитой от переполнения
            float r = static_cast<float>(pixelColor.r) * brightness;
            float g = static_cast<float>(pixelColor.g) * brightness;
            float b = static_cast<float>(pixelColor.b) * brightness;
            
            pixelColor.r = static_cast<sf::Uint8>(std::min(255.0f, r));
            pixelColor.g = static_cast<sf::Uint8>(std::min(255.0f, g));
            pixelColor.b = static_cast<sf::Uint8>(std::min(255.0f, b));
            
            // Применяем цветовой оттенок
            if (obj.tint != sf::Color::White) {
                pixelColor.r = static_cast<sf::Uint8>(pixelColor.r * obj.tint.r / 255);
                pixelColor.g = static_cast<sf::Uint8>(pixelColor.g * obj.tint.g / 255);
                pixelColor.b = static_cast<sf::Uint8>(pixelColor.b * obj.tint.b / 255);
                pixelColor.a = static_cast<sf::Uint8>(pixelColor.a * obj.tint.a / 255);
            }
            
            // Рисуем пиксель
            sf::Vertex pixel(sf::Vector2f(static_cast<float>(x), static_cast<float>(y)), pixelColor);
            renderTexture_.draw(&pixel, 1, sf::Points);
        }
    }
}

bool Renderer::isLightVisible(const sf::Vector2f& lightPos, const sf::Vector2f& targetPos,
                             const std::vector<std::vector<int>>& wallMap) const {
    // Проверка видимости света с учетом геометрии стен
    
    float dx = lightPos.x - targetPos.x;
    float dy = lightPos.y - targetPos.y;
    float distance = std::sqrt(dx*dx + dy*dy);
    
    if (distance < 0.1f) return true;
    
    // Нормализуем вектор направления
    float stepX = dx / distance;
    float stepY = dy / distance;
    
    // Количество шагов зависит от расстояния, но с минимальным/максимальным значением
    int maxSteps = static_cast<int>(distance * 10.0f);
    maxSteps = std::max(5, std::min(50, maxSteps));
    
    float currentX = targetPos.x;
    float currentY = targetPos.y;
    
    float startOffset = 0.15f;
    currentX += stepX * startOffset;
    currentY += stepY * startOffset;
    
    for (int i = 0; i < maxSteps; ++i) {
        int mapX = static_cast<int>(currentX);
        int mapY = static_cast<int>(currentY);
        
        // Проверяем, не является ли клетка стеной
        if (mapY >= 0 && mapY < static_cast<int>(wallMap.size()) &&
            mapX >= 0 && mapX < static_cast<int>(wallMap[0].size())) {
            if (wallMap[mapY][mapX] != 0) {
                return false;
            }
        }
        
        // Переходим к следующей точке
        currentX += stepX * 0.1f;
        currentY += stepY * 0.1f;
        
        // Прерываем, если прошли всё расстояние
        float traveled = (i + 1) * 0.1f + startOffset;
        if (traveled >= distance) {
            break;
        }
    }
    
    return true;
}

bool Renderer::isPointInSameCell(float worldX, float worldY, int cellX, int cellY) const {
    // Проверяет, находится ли точка в той же клетке карты
    return static_cast<int>(worldX) == cellX && 
           static_cast<int>(worldY) == cellY;
}

// Старый метод для совместимости (используется в других частях кода)
void Renderer::drawFurnitureSprite(const FurnitureObject& obj, 
                                   const FurnitureData& data,
                                   const PlayerState& player,
                                   float distance,
                                   float angleToPlayer) {
    // Пустая реализация для совместимости
}

// Старый метод для совместимости
void Renderer::renderFurnitureInternal(const PlayerState& player,
                                      const std::vector<FurnitureObject>& furniture,
                                      const std::unordered_map<std::string, FurnitureData>& furnitureTypes) {
    // Пустая реализация для совместимости
}