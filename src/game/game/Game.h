#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include "engine/raycasting/RayCaster.h"
#include "engine/renderer/Renderer.h"
#include "engine/geographer/MapLoader.h"
#include "input_handler/InputHandler.h"
#include "resource_manager/ResourceManager.h"

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
    std::unique_ptr<Renderer> renderer_; // Теперь как умный указатель
    InputHandler inputHandler_;
    
    // Состояние игры
    PlayerState player_;
    std::vector<std::vector<int>> map_;
    
    void handleEvents();
    void loadResources();
};