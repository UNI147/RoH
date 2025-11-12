#include "Renderer.h"
#include <cmath>

Renderer::Renderer(sf::RenderWindow& window) : window_(window) {}

void Renderer::renderFrame(const PlayerState& player, const std::vector<std::vector<int>>& map,
                          RayCaster& rayCaster) {
    // Рендеринг для каждого вертикального столбца экрана
    for (int x = 0; x < SCREEN_WIDTH; ++x) {
        // Вычисление позиции луча на камере
        float cameraX = 2 * x / float(SCREEN_WIDTH) - 1;
        
        // Бросаем луч
        RayHit hit = rayCaster.castRay(player, map, cameraX);
        
        // Вычисление высоты линии стены для отрисовки
        int lineHeight = static_cast<int>(SCREEN_HEIGHT / hit.distance);
        
        // Вычисление верхней и нижней точек стены
        int drawStart = -lineHeight / 2 + SCREEN_HEIGHT / 2;
        if (drawStart < 0) drawStart = 0;
        int drawEnd = lineHeight / 2 + SCREEN_HEIGHT / 2;
        if (drawEnd >= SCREEN_HEIGHT) drawEnd = SCREEN_HEIGHT - 1;
        
        // Отрисовка пола и потолка
        drawFloorAndCeiling(x, drawStart, drawEnd);
        
        // Отрисовка стены
        drawWallStrip(x, lineHeight, hit.side, hit.wallX);
    }
}

void Renderer::drawWallStrip(int x, int lineHeight, int side, float wallX) {
    // Простая цветовая схема в зависимости от стороны стены
    sf::Color wallColor;
    if (side == 0) {
        wallColor = sf::Color(200, 100, 100); // Красноватый для x-сторон
    } else {
        wallColor = sf::Color(100, 100, 200); // Синеватый для y-сторон
    }
    
    // Затемнение цвета в зависимости от расстояния
    float brightness = std::min(1.0f, 300.0f / (lineHeight * 0.5f));
    wallColor.r *= brightness;
    wallColor.g *= brightness;
    wallColor.b *= brightness;
    
    // Отрисовка вертикальной линии
    sf::Vertex line[] = {
        sf::Vertex(sf::Vector2f(x, SCREEN_HEIGHT / 2 - lineHeight / 2), wallColor),
        sf::Vertex(sf::Vector2f(x, SCREEN_HEIGHT / 2 + lineHeight / 2), wallColor)
    };
    
    window_.draw(line, 2, sf::Lines);
}

void Renderer::drawFloorAndCeiling(int x, int drawStart, int drawEnd) {
    // Пол
    sf::Vertex floorLine[] = {
        sf::Vertex(sf::Vector2f(x, drawEnd), sf::Color(50, 50, 50)),
        sf::Vertex(sf::Vector2f(x, SCREEN_HEIGHT), sf::Color(50, 50, 50))
    };
    
    // Потолок
    sf::Vertex ceilingLine[] = {
        sf::Vertex(sf::Vector2f(x, 0), sf::Color(100, 100, 100)),
        sf::Vertex(sf::Vector2f(x, drawStart), sf::Color(100, 100, 100))
    };
    
    window_.draw(floorLine, 2, sf::Lines);
    window_.draw(ceilingLine, 2, sf::Lines);
}