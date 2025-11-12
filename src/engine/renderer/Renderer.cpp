#include "Renderer.h"
#include <cmath>

Renderer::Renderer(sf::RenderWindow& window) 
    : window_(window) {
    
    // Создаем текстуру для рендеринга с нужным разрешением
    if (renderTexture_.create(RENDER_WIDTH, RENDER_HEIGHT)) {
        renderSprite_.setTexture(renderTexture_.getTexture());
        updateRenderSpriteScale();
        
        // Отключаем сглаживание для пиксель-перфект отображения
        renderSprite_.setTextureRect(sf::IntRect(0, 0, RENDER_WIDTH, RENDER_HEIGHT));
    }
}

void Renderer::updateRenderSpriteScale() {
    sf::Vector2u windowSize = window_.getSize();
    
    // Вычисляем масштаб с сохранением соотношения сторон
    float scaleX = static_cast<float>(windowSize.x) / RENDER_WIDTH;
    float scaleY = static_cast<float>(windowSize.y) / RENDER_HEIGHT;
    
    // Используем минимальный масштаб, чтобы сохранить соотношение сторон
    float scale = std::min(scaleX, scaleY);
    
    renderSprite_.setScale(scale, scale);
    
    // Центрируем спрайт на экране
    float offsetX = (windowSize.x - RENDER_WIDTH * scale) / 2.0f;
    float offsetY = (windowSize.y - RENDER_HEIGHT * scale) / 2.0f;
    renderSprite_.setPosition(offsetX, offsetY);
}

void Renderer::renderFrame(const PlayerState& player, const std::vector<std::vector<int>>& map,
                          RayCaster& rayCaster) {
    // Обновляем масштаб на случай изменения размера окна
    updateRenderSpriteScale();
    
    // Рендерим во внутреннюю текстуру
    renderTexture_.clear();
    
    // Рендеринг для каждого вертикального столбца экрана
    for (int x = 0; x < RENDER_WIDTH; ++x) {
        // Вычисление позиции луча на камере
        float cameraX = 2 * x / float(RENDER_WIDTH) - 1;
        
        // Бросаем луч
        RayHit hit = rayCaster.castRay(player, map, cameraX);
        
        // Вычисление высоты линии стены для отрисовки
        int lineHeight = static_cast<int>(RENDER_HEIGHT / hit.distance);
        
        // Вычисление верхней и нижней точек стены
        int drawStart = -lineHeight / 2 + RENDER_HEIGHT / 2;
        if (drawStart < 0) drawStart = 0;
        int drawEnd = lineHeight / 2 + RENDER_HEIGHT / 2;
        if (drawEnd >= RENDER_HEIGHT) drawEnd = RENDER_HEIGHT - 1;
        
        // Отрисовка пола и потолка
        drawFloorAndCeiling(x, drawStart, drawEnd);
        
        // Отрисовка стены
        drawWallStrip(x, lineHeight, hit.side, hit.wallX);
    }
    
    renderTexture_.display();
    
    // Очищаем окно и отображаем спрайт по центру
    window_.clear(sf::Color::Black);
    window_.draw(renderSprite_);
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
    
    // Исправляем предупреждения компиляции - явное преобразование в sf::Uint8
    wallColor.r = static_cast<sf::Uint8>(wallColor.r * brightness);
    wallColor.g = static_cast<sf::Uint8>(wallColor.g * brightness);
    wallColor.b = static_cast<sf::Uint8>(wallColor.b * brightness);
    
    // Отрисовка вертикальной линии
    sf::Vertex line[] = {
        sf::Vertex(sf::Vector2f(static_cast<float>(x), static_cast<float>(RENDER_HEIGHT / 2 - lineHeight / 2)), wallColor),
        sf::Vertex(sf::Vector2f(static_cast<float>(x), static_cast<float>(RENDER_HEIGHT / 2 + lineHeight / 2)), wallColor)
    };
    
    renderTexture_.draw(line, 2, sf::Lines);
}

void Renderer::drawFloorAndCeiling(int x, int drawStart, int drawEnd) {
    // Пол
    sf::Vertex floorLine[] = {
        sf::Vertex(sf::Vector2f(static_cast<float>(x), static_cast<float>(drawEnd)), sf::Color(50, 50, 50)),
        sf::Vertex(sf::Vector2f(static_cast<float>(x), static_cast<float>(RENDER_HEIGHT)), sf::Color(50, 50, 50))
    };
    
    // Потолок
    sf::Vertex ceilingLine[] = {
        sf::Vertex(sf::Vector2f(static_cast<float>(x), 0.0f), sf::Color(100, 100, 100)),
        sf::Vertex(sf::Vector2f(static_cast<float>(x), static_cast<float>(drawStart)), sf::Color(100, 100, 100))
    };
    
    renderTexture_.draw(floorLine, 2, sf::Lines);
    renderTexture_.draw(ceilingLine, 2, sf::Lines);
}