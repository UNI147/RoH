#include "Game.h"

Game::Game(sf::RenderWindow& window) 
    : window_(window)
    , renderer_(window) {
    
    // Инициализация игрока
    player_.position = sf::Vector2f(1.5f, 1.5f);
    player_.direction = sf::Vector2f(-1.0f, 0.0f);
    player_.plane = sf::Vector2f(0.0f, 0.66f);

    // Загрузка карты
    map_ = mapLoader_.createTestMap();
}

void Game::update() {
    float deltaTime = clock_.restart().asSeconds();
    
    handleEvents();
    inputHandler_.handleInput(player_, deltaTime, map_);
}

void Game::render() {
    window_.clear();
    renderer_.renderFrame(player_, map_, rayCaster_);
    window_.display();
}

void Game::handleEvents() {
    sf::Event event;
    while (window_.pollEvent(event)) {
        if (event.type == sf::Event::Closed)
            window_.close();
        
        // Выход по Escape
        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape)
            window_.close();
    }
}