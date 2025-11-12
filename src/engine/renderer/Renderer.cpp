#include "Renderer.h"
#include <cmath>
#include <iostream>
#include "resource_manager/ResourceManager.h"
#include "resource_manager/TextureNames.h"

Renderer::Renderer(sf::RenderWindow& window) 
    : window_(window) {
    
    if (renderTexture_.create(RENDER_WIDTH, RENDER_HEIGHT)) {
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
            drawTexturedWallStrip(x, drawStart, drawEnd, hit, player);
        } else {
            drawWallStrip(x, drawStart, drawEnd, hit.side, hit.distance);
        }
    }
    
    renderTexture_.display();
    window_.clear(sf::Color::Black);
    window_.draw(renderSprite_);
}

void Renderer::drawTexturedWallStrip(int x, int drawStart, int drawEnd, const RayHit& hit, const PlayerState& player) {
    int lineHeight = drawEnd - drawStart;
    if (lineHeight <= 0) return;
    
    // Правильное вычисление координаты текстуры по X
    float wallX;
    if (hit.side == 0) {
        wallX = player.position.y + hit.distance * hit.rayDirY;
    } else {
        wallX = player.position.x + hit.distance * hit.rayDirX;
    }
    wallX -= std::floor(wallX);
    
    int texX = static_cast<int>(wallX * wallTextureImage_.getSize().x);
    
    // Корректировка для предотвращения зеркального отображения
    if ((hit.side == 0 && hit.rayDirX > 0) || (hit.side == 1 && hit.rayDirY < 0)) {
        texX = wallTextureImage_.getSize().x - texX - 1;
    }
    
    // Обеспечиваем, чтобы texX был в пределах текстуры
    texX = std::max(0, std::min(texX, static_cast<int>(wallTextureImage_.getSize().x) - 1));
    
    // Создаем VertexArray для всей линии
    sf::VertexArray vertices(sf::Points, lineHeight);
    
    float step = static_cast<float>(wallTextureImage_.getSize().y) / lineHeight;
    float texPos = (drawStart - RENDER_HEIGHT / 2.0f + lineHeight / 2.0f) * step;
    
    for (int i = 0; i < lineHeight; ++i) {
        int y = drawStart + i;
        int texY = static_cast<int>(texPos);
        
        // Корректное ограничение координаты текстуры
        if (texY < 0) texY = 0;
        if (texY >= static_cast<int>(wallTextureImage_.getSize().y)) 
            texY = wallTextureImage_.getSize().y - 1;
        
        texPos += step;
        
        sf::Color pixelColor = wallTextureImage_.getPixel(texX, texY);
        
        // Затемнение для сторон Y для создания эффекта глубины
        if (hit.side == 1) {
            pixelColor.r = static_cast<sf::Uint8>(pixelColor.r * 0.7f);
            pixelColor.g = static_cast<sf::Uint8>(pixelColor.g * 0.7f);
            pixelColor.b = static_cast<sf::Uint8>(pixelColor.b * 0.7f);
        }
        
        // Дополнительное затемнение в зависимости от расстояния
        float brightness = std::min(1.0f, 8.0f / hit.distance);
        pixelColor.r = static_cast<sf::Uint8>(pixelColor.r * brightness);
        pixelColor.g = static_cast<sf::Uint8>(pixelColor.g * brightness);
        pixelColor.b = static_cast<sf::Uint8>(pixelColor.b * brightness);
        
        vertices[i].position = sf::Vector2f(static_cast<float>(x), static_cast<float>(y));
        vertices[i].color = pixelColor;
    }
    
    renderTexture_.draw(vertices);
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
        float posZ = 0.5f * RENDER_HEIGHT;
        
        // Горизонтальное расстояние от камеры к полу для текущего ряда
        float rowDistance = posZ / p;
        
        // Вычисляем реальные шаги
        float floorStepX = rowDistance * (rayDirX1 - rayDirX0) / RENDER_WIDTH;
        float floorStepY = rowDistance * (rayDirY1 - rayDirY0) / RENDER_WIDTH;
        
        // Реальная координата пола
        float floorX = player.position.x + rowDistance * rayDirX0;
        float floorY = player.position.y + rowDistance * rayDirY0;
        
        for (int x = 0; x < RENDER_WIDTH; ++x) {
            // Координаты текстуры - ИСПРАВЛЕНО: убрана битовая операция
            int tx = static_cast<int>(floorTextureImage_.getSize().x * (floorX - std::floor(floorX)));
            int ty = static_cast<int>(floorTextureImage_.getSize().y * (floorY - std::floor(floorY)));
            
            // Ограничиваем координаты текстуры
            tx = std::max(0, std::min(tx, static_cast<int>(floorTextureImage_.getSize().x) - 1));
            ty = std::max(0, std::min(ty, static_cast<int>(floorTextureImage_.getSize().y) - 1));
            
            floorX += floorStepX;
            floorY += floorStepY;
            
            // Цвет пола
            sf::Color floorColor = floorTextureImage_.getPixel(tx, ty);
            float floorBrightness = std::min(1.0f, 3.0f / rowDistance);
            floorColor.r = static_cast<sf::Uint8>(floorColor.r * floorBrightness);
            floorColor.g = static_cast<sf::Uint8>(floorColor.g * floorBrightness);
            floorColor.b = static_cast<sf::Uint8>(floorColor.b * floorBrightness);
            
            // Отрисовка пикселя пола
            sf::Vertex floorPixel(sf::Vector2f(static_cast<float>(x), static_cast<float>(y)), floorColor);
            renderTexture_.draw(&floorPixel, 1, sf::Points);
            
            // Симметрично рендерим потолок
            int ceilingY = RENDER_HEIGHT - y - 1;
            if (ceilingY >= 0) {
                sf::Color ceilingColor = ceilingTextureImage_.getPixel(tx, ty);
                float ceilingBrightness = std::min(1.0f, 2.0f / rowDistance);
                ceilingColor.r = static_cast<sf::Uint8>(ceilingColor.r * ceilingBrightness);
                ceilingColor.g = static_cast<sf::Uint8>(ceilingColor.g * ceilingBrightness);
                ceilingColor.b = static_cast<sf::Uint8>(ceilingColor.b * ceilingBrightness);
                
                sf::Vertex ceilingPixel(sf::Vector2f(static_cast<float>(x), static_cast<float>(ceilingY)), ceilingColor);
                renderTexture_.draw(&ceilingPixel, 1, sf::Points);
            }
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
    float scaleX = static_cast<float>(windowSize.x) / RENDER_WIDTH;
    float scaleY = static_cast<float>(windowSize.y) / RENDER_HEIGHT;
    float scale = std::min(scaleX, scaleY);
    
    renderSprite_.setScale(scale, scale);
    
    float offsetX = (windowSize.x - RENDER_WIDTH * scale) / 2.0f;
    float offsetY = (windowSize.y - RENDER_HEIGHT * scale) / 2.0f;
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
    wallColor.r = static_cast<sf::Uint8>(wallColor.r * brightness);
    wallColor.g = static_cast<sf::Uint8>(wallColor.g * brightness);
    wallColor.b = static_cast<sf::Uint8>(wallColor.b * brightness);
    
    sf::Vertex line[] = {
        sf::Vertex(sf::Vector2f(static_cast<float>(x), static_cast<float>(drawStart)), wallColor),
        sf::Vertex(sf::Vector2f(static_cast<float>(x), static_cast<float>(drawEnd)), wallColor)
    };
    
    renderTexture_.draw(line, 2, sf::Lines);
}

void Renderer::drawFloorAndCeiling(int x, int drawStart, int drawEnd) {
}