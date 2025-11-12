#include <SFML/Graphics.hpp>
#include "engine/raycasting/RayCaster.h"
#include "engine/renderer/Renderer.h"
#include "engine/geographer/MapLoader.h"
#include "input_handler/InputHandler.h"

int main() {
    // Создание окна
    sf::RenderWindow window(sf::VideoMode(800, 600), "RoH Demo");
    window.setFramerateLimit(30);

    // Инициализация систем
    MapLoader mapLoader;
    RayCaster rayCaster;
    Renderer renderer(window);
    InputHandler inputHandler;
    
    PlayerState player;
    player.position = sf::Vector2f(1.5f, 1.5f);
    player.direction = sf::Vector2f(-1.0f, 0.0f);
    player.plane = sf::Vector2f(0.0f, 0.66f);

    // Загрузка карты
    std::vector<std::vector<int>> map = mapLoader.createTestMap();

    // Основной игровой цикл
    sf::Clock clock;
    while (window.isOpen()) {
        float deltaTime = clock.restart().asSeconds();
        
        // Обработка событий
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
            
            // Выход по Escape
            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape)
                window.close();
        }

        // Обновление ввода
        inputHandler.handleInput(player, deltaTime, map);
        
        // Очистка экрана
        window.clear();

        // Рендеринг
        renderer.renderFrame(player, map, rayCaster);

        // Отображение
        window.display();
    }

    return 0;
}