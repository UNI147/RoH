#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <unordered_map>

// Данные мебели из .fur файла
struct FurnitureData {
    std::string name;
    std::string textureName;
    float width = 1.0f;
    float height = 1.0f;
    bool passable = false;
    bool transparent = false;
    float yOffset = 0.0f;
    sf::Vector2f pivot = {0.5f, 1.0f};
    
    // Свойства для столкновений
    struct CollisionBox {
        float x = 0.0f;
        float y = 0.0f;
        float width = 1.0f;
        float height = 1.0f;
    } collisionBox;
};

// Объект мебели на уровне
struct FurnitureObject {
    std::string furnitureType;
    sf::Vector2f position;
    float rotation = 0.0f;
    float scale = 1.0f;
    sf::Color tint = sf::Color::White;
};