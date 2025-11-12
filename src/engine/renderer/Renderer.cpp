#include "Renderer.h"
#include <cmath>
#include <iostream>
#include <algorithm>
#include "resource_manager/ResourceManager.h"
#include "resource_manager/TextureNames.h"

Renderer::Renderer(sf::RenderWindow& window) 
    : window_(window) {
    
    if (renderTexture_.create(RENDER_WIDTH, RENDER_HEIGHT)) {
        renderTexture_.setSmooth(false);
        renderSprite_.setTexture(renderTexture_.getTexture());
        updateRenderSpriteScale();
        renderSprite_.setTextureRect(sf::IntRect(0, 0, RENDER_WIDTH, RENDER_HEIGHT));
    }
    
    auto& rm = ResourceManager::getInstance();
    useTextures_ = true;
    
    if (!rm.hasTexture(Textures::BRICKS)) {
        std::cout << "Texture not available: " << Textures::BRICKS << std::endl;
        useTextures_ = false;
    }
    if (!rm.hasTexture(Textures::BOARDS)) {
        std::cout << "Texture not available: " << Textures::BOARDS << std::endl;
        useTextures_ = false;
    }
    if (!rm.hasTexture(Textures::PARQUET)) {
        std::cout << "Texture not available: " << Textures::PARQUET << std::endl;
        useTextures_ = false;
    }
    
    if (useTextures_) {
        std::cout << "Textured rendering enabled" << std::endl;
        
        try {
            wallTextureImage_ = rm.getTexture(Textures::BRICKS).copyToImage();
            floorTextureImage_ = rm.getTexture(Textures::PARQUET).copyToImage();
            ceilingTextureImage_ = rm.getTexture(Textures::BOARDS).copyToImage();
        } catch (const std::exception& e) {
            std::cerr << "Error preloading texture images: " << e.what() << std::endl;
            useTextures_ = false;
        }
    } else {
        std::cout << "Falling back to color rendering" << std::endl;
    }
}

void Renderer::renderFrame(const PlayerState& player, const std::vector<std::vector<int>>& map,
                          RayCaster& rayCaster) {
    updateRenderSpriteScale();
    renderTexture_.clear();
    
    // Сначала рендерим пол и потолок
    if (useTextures_) {
        drawTexturedFloorAndCeiling(player, map, rayCaster);
    } else {
        drawSolidFloorAndCeiling();
    }
    
    // Затем рендерим стены поверх
    for (int x = 0; x < RENDER_WIDTH; ++x) {
        float cameraX = 2 * x / float(RENDER_WIDTH) - 1;
        RayHit hit = rayCaster.castRay(player, map, cameraX);
        
        int lineHeight = static_cast<int>(RENDER_HEIGHT / hit.distance);
        int drawStart = -lineHeight / 2 + RENDER_HEIGHT / 2;
        if (drawStart < 0) drawStart = 0;
        int drawEnd = lineHeight / 2 + RENDER_HEIGHT / 2;
        if (drawEnd >= RENDER_HEIGHT) drawEnd = RENDER_HEIGHT - 1;
        
        if (useTextures_) {
            drawTexturedWallStrip(x, drawStart, drawEnd, hit, lineHeight);
        } else {
            drawWallStrip(x, drawStart, drawEnd, hit.side, hit.distance);
        }
    }
    
    renderTexture_.display();
    window_.clear(sf::Color::Black);
    window_.draw(renderSprite_);
}

void Renderer::drawTexturedWallStrip(int x, int drawStart, int drawEnd, const RayHit& hit, int lineHeight) {
    if (lineHeight <= 0) return;
    
    // Вычисление координаты текстуры по X
    int texX = static_cast<int>(hit.wallX * static_cast<float>(wallTextureImage_.getSize().x));
    
    // Корректировка для предотвращения зеркального отображения
    if ((hit.side == 0 && hit.rayDirX > 0) || (hit.side == 1 && hit.rayDirY < 0)) {
        texX = wallTextureImage_.getSize().x - texX - 1;
    }
    
    // Обеспечиваем, чтобы texX был в пределах текстуры
    texX = std::max(0, std::min(texX, static_cast<int>(wallTextureImage_.getSize().x) - 1));
    
    // Правильное вычисление шага текстуры по Y
    float step = static_cast<float>(wallTextureImage_.getSize().y) / static_cast<float>(lineHeight);
    float texPos = (static_cast<float>(drawStart) - RENDER_HEIGHT / 2.0f + static_cast<float>(lineHeight) / 2.0f) * step;
    
    // Применяем затемнение в зависимости от расстояния и стороны
    float brightness = std::min(1.0f, 8.0f / hit.distance);
    if (hit.side == 1) {
        brightness *= 0.8f; // Более темный цвет для Y-сторон
    }
    
    for (int y = drawStart; y < drawEnd; ++y) {
        int texY = static_cast<int>(texPos) % wallTextureImage_.getSize().y;
        texY = std::max(0, std::min(texY, static_cast<int>(wallTextureImage_.getSize().y) - 1));
        
        sf::Color pixelColor = wallTextureImage_.getPixel(texX, texY);
        
        // Применяем затемнение
        pixelColor.r = static_cast<sf::Uint8>(static_cast<float>(pixelColor.r) * brightness);
        pixelColor.g = static_cast<sf::Uint8>(static_cast<float>(pixelColor.g) * brightness);
        pixelColor.b = static_cast<sf::Uint8>(static_cast<float>(pixelColor.b) * brightness);
        
        // Рисуем один пиксель
        sf::Vertex pixel(sf::Vector2f(static_cast<float>(x), static_cast<float>(y)), pixelColor);
        renderTexture_.draw(&pixel, 1, sf::Points);
        
        texPos += step;
    }
}

void Renderer::drawTexturedFloorAndCeiling(const PlayerState& player, const std::vector<std::vector<int>>& map,
                                          RayCaster& rayCaster) {
    // Рендерим пол и потолок для всего экрана
    for (int y = RENDER_HEIGHT / 2 + 1; y < RENDER_HEIGHT; ++y) {
        // Луч для пола
        float rayDirX0 = player.direction.x - player.plane.x;
        float rayDirY0 = player.direction.y - player.plane.y;
        float rayDirX1 = player.direction.x + player.plane.x;
        float rayDirY1 = player.direction.y + player.plane.y;
        
        // Текущая позиция ряда пикселей
        int p = y - RENDER_HEIGHT / 2;
        
        // Вертикальная позиция камеры
        float posZ = 0.5f * static_cast<float>(RENDER_HEIGHT);
        
        // Горизонтальное расстояние от камеры к полу для текущего ряда
        float rowDistance = posZ / static_cast<float>(p);
        
        // Вычисляем реальные шаги
        float floorStepX = rowDistance * (rayDirX1 - rayDirX0) / static_cast<float>(RENDER_WIDTH);
        float floorStepY = rowDistance * (rayDirY1 - rayDirY0) / static_cast<float>(RENDER_WIDTH);
        
        // Реальная координата пола
        float floorX = player.position.x + rowDistance * rayDirX0;
        float floorY = player.position.y + rowDistance * rayDirY0;
        
        for (int x = 0; x < RENDER_WIDTH; ++x) {
            // Координаты текстуры для пола - ИНВЕРТИРУЕМ по X чтобы исправить отзеркаливание
            int floorTexX = static_cast<int>(static_cast<float>(floorTextureImage_.getSize().x) * (1.0f - (floorX - std::floor(floorX))));
            int floorTexY = static_cast<int>(static_cast<float>(floorTextureImage_.getSize().y) * (floorY - std::floor(floorY)));
            
            // Ограничиваем координаты текстуры
            floorTexX = std::max(0, std::min(floorTexX, static_cast<int>(floorTextureImage_.getSize().x) - 1));
            floorTexY = std::max(0, std::min(floorTexY, static_cast<int>(floorTextureImage_.getSize().y) - 1));
            
            // Цвет пола
            sf::Color floorColor = floorTextureImage_.getPixel(floorTexX, floorTexY);
            float floorBrightness = std::min(1.0f, 3.0f / rowDistance);
            floorColor.r = static_cast<sf::Uint8>(static_cast<float>(floorColor.r) * floorBrightness);
            floorColor.g = static_cast<sf::Uint8>(static_cast<float>(floorColor.g) * floorBrightness);
            floorColor.b = static_cast<sf::Uint8>(static_cast<float>(floorColor.b) * floorBrightness);
            
            // Отрисовка пикселя пола
            sf::Vertex floorPixel(sf::Vector2f(static_cast<float>(x), static_cast<float>(y)), floorColor);
            renderTexture_.draw(&floorPixel, 1, sf::Points);
            
            // Симметрично рендерим потолок с отдельной текстурой
            int ceilingY = RENDER_HEIGHT - y - 1;
            if (ceilingY >= 0) {
                // Координаты текстуры для потолка (используем отдельную текстуру)
                int ceilingTexX = static_cast<int>(static_cast<float>(ceilingTextureImage_.getSize().x) * (floorX - std::floor(floorX)));
                int ceilingTexY = static_cast<int>(static_cast<float>(ceilingTextureImage_.getSize().y) * (floorY - std::floor(floorY)));
                
                // Ограничиваем координаты текстуры
                ceilingTexX = std::max(0, std::min(ceilingTexX, static_cast<int>(ceilingTextureImage_.getSize().x) - 1));
                ceilingTexY = std::max(0, std::min(ceilingTexY, static_cast<int>(ceilingTextureImage_.getSize().y) - 1));
                
                sf::Color ceilingColor = ceilingTextureImage_.getPixel(ceilingTexX, ceilingTexY);
                float ceilingBrightness = std::min(1.0f, 2.0f / rowDistance);
                ceilingColor.r = static_cast<sf::Uint8>(static_cast<float>(ceilingColor.r) * ceilingBrightness);
                ceilingColor.g = static_cast<sf::Uint8>(static_cast<float>(ceilingColor.g) * ceilingBrightness);
                ceilingColor.b = static_cast<sf::Uint8>(static_cast<float>(ceilingColor.b) * ceilingBrightness);
                
                sf::Vertex ceilingPixel(sf::Vector2f(static_cast<float>(x), static_cast<float>(ceilingY)), ceilingColor);
                renderTexture_.draw(&ceilingPixel, 1, sf::Points);
            }
            
            floorX += floorStepX;
            floorY += floorStepY;
        }
    }
}

void Renderer::drawSolidFloorAndCeiling() {
    // Простая заливка пола и потолка цветом
    for (int x = 0; x < RENDER_WIDTH; ++x) {
        // Пол
        sf::Vertex floorLine[] = {
            sf::Vertex(sf::Vector2f(static_cast<float>(x), static_cast<float>(RENDER_HEIGHT / 2)), sf::Color(50, 50, 50)),
            sf::Vertex(sf::Vector2f(static_cast<float>(x), static_cast<float>(RENDER_HEIGHT)), sf::Color(50, 50, 50))
        };
        
        // Потолок
        sf::Vertex ceilingLine[] = {
            sf::Vertex(sf::Vector2f(static_cast<float>(x), 0.0f), sf::Color(100, 100, 100)),
            sf::Vertex(sf::Vector2f(static_cast<float>(x), static_cast<float>(RENDER_HEIGHT / 2)), sf::Color(100, 100, 100))
        };
        
        renderTexture_.draw(floorLine, 2, sf::Lines);
        renderTexture_.draw(ceilingLine, 2, sf::Lines);
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
    
    float brightness = std::min(1.0f, 5.0f / distance);
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
}