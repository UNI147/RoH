#include <SFML/Graphics.hpp>
#include "game/game/Game.h"

int main() {
    // Создание полноэкранного окна
    sf::VideoMode desktopMode = sf::VideoMode::getDesktopMode();
    sf::RenderWindow window(desktopMode, "RoH Demo", sf::Style::Fullscreen);
    window.setFramerateLimit(30);

    // Инициализация игры
    Game game(window);

    // Основной игровой цикл
    while (window.isOpen()) {
        game.update();
        game.render();
    }

    return 0;
}