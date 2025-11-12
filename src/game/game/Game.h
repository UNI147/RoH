#pragma once
#include <SFML/Graphics.hpp>
#include "engine/raycasting/RayCaster.h"
#include "engine/renderer/Renderer.h"
#include "engine/geographer/MapLoader.h"
#include "input_handler/InputHandler.h"

class Game {
public:
    Game(sf::RenderWindow& window);
    void update();
    void render();

private:
    sf::RenderWindow& window_;
    sf::Clock clock_;
    
    // Системы игры
    MapLoader mapLoader_;
    RayCaster rayCaster_;
    Renderer renderer_;
    InputHandler inputHandler_;
    
    // Состояние игры
    PlayerState player_;
    std::vector<std::vector<int>> map_;
    
    void handleEvents();
};