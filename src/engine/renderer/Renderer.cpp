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

float Renderer::calculateLightAtPoint(const sf::Vector2f& point, 
                                    const std::vector<std::vector<int>>& wallMap) const {
    float totalLight = 0.0f;
    
    for (const auto& light : lightSources_) {
        float dx = point.x - light.position.x;
        float dy = point.y - light.position.y;
        float lightDist = std::sqrt(dx*dx + dy*dy);
        
        if (lightDist < light.radius) {
            // Упрощенная проверка видимости - используем саму точку
            if (isLightVisible(light.position, point, wallMap)) {
                // Квадратичное затухание
                float lightIntensity = 1.0f - (lightDist / light.radius);
                lightIntensity = lightIntensity * lightIntensity;
                lightIntensity *= light.intensity;
                
                // ДЛЯ ПОЛА И ПОТОЛКА - ВСЕГДА ПОЛНАЯ ЯРКОСТЬ (угол не учитываем)
                totalLight += lightIntensity;
            }
        }
    }
    
    return std::min(2.0f, totalLight);
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
    float texCoordX = hit.wallX * static_cast<float>(texWidth);
    int texX = static_cast<int>(texCoordX);
    
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
    
    // Мягкое затемнение по расстоянию
    float distanceFactor = std::min(1.0f, hit.distance / maxDarkDistance_);
    float distanceBrightness = 1.0f - (distanceFactor * distanceFactor);
    
    // Дополнительное затемнение для Y-сторон
    float sideDarkness = 1.0f;
    if (hit.side == 1) {
        sideDarkness = 0.8f;
    }
    
    // Базовое затемнение
    float baseBrightness = distanceBrightness * sideDarkness;
    
    // Расчет освещения для стен
    float lightBrightness = 0.0f;
    if (!lightSources_.empty() && currentWallMap_) {
        
        // Используем точку на стене для проверки освещения
        float wallX, wallY;
        
        if (hit.side == 0) {
            wallX = hit.mapX + (hit.rayDirX > 0 ? 0.0f : 1.0f);
            wallY = hit.mapY + hit.wallX;
        } else { // Горизонтальная стена
            wallX = hit.mapX + hit.wallX;
            wallY = hit.mapY + (hit.rayDirY > 0 ? 0.0f : 1.0f);
        }
        
        // Для стен проверяем несколько точек по высоте
        const int samplePoints = 3;
        float totalLight = 0.0f;
        
        // ИСПРАВЛЕННЫЙ ЦИКЛ: были серьезные синтаксические ошибки
        for (int i = 0; i < samplePoints; ++i) {
            float offset = (i - samplePoints/2) * 0.1f;
            
            float sampleX = wallX;
            float sampleY = wallY;
            
            // Смещаем точку вдоль стены
            if (hit.side == 0) {
                sampleY += offset;
            } else {
                sampleX += offset;
            }
            
            // НЕМНОГО СДВИГАЕМ ОТ СТЕНЫ, ЧТОБЫ НЕ УПИРАТЬСЯ В НЕЕ
            if (hit.side == 0) {
                sampleX += (hit.rayDirX > 0 ? -0.05f : 0.05f);
            } else {
                sampleY += (hit.rayDirY > 0 ? -0.05f : 0.05f);
            }
            
            // Используем упрощенный метод для стен
            float pointLight = calculateLightAtPoint(sf::Vector2f(sampleX, sampleY), *currentWallMap_);
            
            // Для стен учитываем угол падения
            for (const auto& light : lightSources_) {
                float ldx = sampleX - light.position.x;
                float ldy = sampleY - light.position.y;
                float ldist = std::sqrt(ldx*ldx + ldy*ldy);
                
                if (ldist < light.radius && isLightVisible(light.position, sf::Vector2f(sampleX, sampleY), *currentWallMap_)) {
                    if (ldist > 0.1f) {
                        float nx = ldx / ldist;
                        float ny = ldy / ldist;
                        
                        // Учет угла для стен
                        float angleFactor;
                        if (hit.side == 0) {
                            // Вертикальная стена - нормаль по X
                            float dot = std::abs(nx);
                            angleFactor = 0.7f + 0.3f * dot;
                        } else {
                            // Горизонтальная стена - нормаль по Y
                            float dot = std::abs(ny);
                            angleFactor = 0.7f + 0.3f * dot;
                        }
                        
                        // Применяем угол к интенсивности
                        float intensity = 1.0f - (ldist / light.radius);
                        intensity = intensity * intensity;
                        intensity *= angleFactor;
                        intensity *= light.intensity;
                        
                        pointLight = std::max(pointLight, intensity);
                    }
                }
            }
            
            totalLight += pointLight;
        }
        
        lightBrightness = totalLight / static_cast<float>(samplePoints);
        lightBrightness *= wallLightFactor_;
    }

    // ИТОГОВАЯ ЯРКОСТЬ
    float brightness = baseBrightness * 0.5f + lightBrightness * 1.5f;
    brightness = std::max(0.0f, std::min(3.0f, brightness));
    
    for (int y = drawStart; y < drawEnd; ++y) {
        int texY = static_cast<int>(texPos);
        if (texY >= textureImage->getSize().y) texY = textureImage->getSize().y - 1;
        if (texY < 0) texY = 0;
        texY = std::max(0, std::min(texY, static_cast<int>(textureImage->getSize().y) - 1));
        
        sf::Color pixelColor = textureImage->getPixel(texX, texY);
        
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
                    
                    // ОСВЕЩЕНИЕ ДЛЯ ПОЛА
                    float floorLightBrightness = 0.0f;
                    if (!lightSources_.empty() && wallMapPtr) {
                        // Используем универсальный метод с текущими координатами floorX и floorY
                        floorLightBrightness = calculateLightAtPoint(
                            sf::Vector2f(floorX, floorY), 
                            *wallMapPtr
                        );
                        floorLightBrightness *= floorLightFactor_;
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
                        // Используем универсальный метод с текущими координатами ceilingX и ceilingY
                        ceilingLightBrightness = calculateLightAtPoint(
                            sf::Vector2f(ceilingX, ceilingY), 
                            *wallMapPtr
                        );
                        ceilingLightBrightness *= ceilingLightFactor_;
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
    
    // Теперь рендерим мебель с учетом буфера глубина
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
    int yOffset = static_cast<int>(data.yOffset * static_cast<float>(spriteHeight));
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
    if (!lightSources_.empty() && wallMapPtr) {
        // Используем универсальный метод
        lightBrightness = calculateLightAtPoint(obj.position, *wallMapPtr);
        lightBrightness *= furnitureLightFactor_;
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
            
            int texXInt = 0, texYInt = 0;
            
            // Применяем вращение если нужно
            if (useRotation) {
                float relX = texX - centerX;
                float relY = texY - centerY;
                
                float rotatedX = relX * cosA - relY * sinA + centerX;
                float rotatedY = relX * sinA + relY * cosA + centerY;
                
                // Проверяем границы после вращения
                if (rotatedX >= 0 && rotatedX < texWidth &&
                    rotatedY >= 0 && rotatedY < texHeight) {
                    texXInt = static_cast<int>(std::round(rotatedX));
                    texYInt = static_cast<int>(std::round(rotatedY));
                } else {
                    continue;
                }
            } else {
                // Прямое преобразование
                texXInt = static_cast<int>(std::round(texX));
                texYInt = static_cast<int>(std::round(texY));
            }
            
            // Проверяем границы текстуры
            if (texXInt < 0 || texXInt >= texWidth || 
                texYInt < 0 || texYInt >= texHeight) {
                continue;
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

bool Renderer::isLightVisible(const sf::Vector2f& lightPos, 
                             const sf::Vector2f& targetPos,
                             const std::vector<std::vector<int>>& wallMap) const {
    float dx = targetPos.x - lightPos.x;
    float dy = targetPos.y - lightPos.y;
    float distance = std::sqrt(dx*dx + dy*dy);
    
    if (distance < 0.1f) return true;
    
    // Нормализуем вектор направления
    float stepX = dx / distance;
    float stepY = dy / distance;
    
    // Более грубый шаг для производительности
    float stepSize = 0.1f;
    int steps = static_cast<int>(distance / stepSize) + 1;
    
    float currentX = lightPos.x;
    float currentY = lightPos.y;
    
    // Игнорируем начальную и конечную точки
    for (int i = 1; i < steps; ++i) {
        currentX += stepX * stepSize;
        currentY += stepY * stepSize;
        
        // Если мы достигли или превысили целевую точку, выходим
        if (i >= steps - 1) break;
        
        int mapX = static_cast<int>(currentX);
        int mapY = static_cast<int>(currentY);
        
        // Проверяем границы карты
        if (mapY >= 0 && mapY < static_cast<int>(wallMap.size()) &&
            mapX >= 0 && mapX < static_cast<int>(wallMap[0].size())) {
            
            // Если это стена, свет заблокирован
            if (wallMap[mapY][mapX] != 0) {
                // Но проверяем, не находимся ли мы уже в целевой клетке
                int targetMapX = static_cast<int>(targetPos.x);
                int targetMapY = static_cast<int>(targetPos.y);
                
                // Если это не целевая клетка, свет заблокирован
                if (!(mapX == targetMapX && mapY == targetMapY)) {
                    return false;
                }
            }
        }
    }
    
    return true;
}

bool Renderer::isPointInSameCell(float worldX, float worldY, int cellX, int cellY) const {
    // Проверяет, находится ли точка в той же клетке карты
    return static_cast<int>(worldX) == cellX && 
           static_cast<int>(worldY) == cellY;
}

void Renderer::collectLightSourcesForScene(const std::vector<FurnitureObject>& furniture,
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