#include "InputHandler.h"
#include <cmath>

void InputHandler::handleInput(PlayerState& player, float deltaTime, 
                              const std::vector<std::vector<int>>& map) {
    // Сохраняем старую позицию для отката при коллизии
    sf::Vector2f oldPosition = player.position;
    
    // Движение вперед/назад
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) {
        sf::Vector2f newPos = player.position;
        newPos.x += player.direction.x * MOVE_SPEED * deltaTime;
        newPos.y += player.direction.y * MOVE_SPEED * deltaTime;
        
        if (canMoveTo(player, newPos, map)) {
            player.position = newPos;
        }
    }
    
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
        sf::Vector2f newPos = player.position;
        newPos.x -= player.direction.x * MOVE_SPEED * deltaTime;
        newPos.y -= player.direction.y * MOVE_SPEED * deltaTime;
        
        if (canMoveTo(player, newPos, map)) {
            player.position = newPos;
        }
    }
    
    // Стрэйф влево/вправо
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
        sf::Vector2f newPos = player.position;
        newPos.x += player.direction.y * MOVE_SPEED * deltaTime;
        newPos.y -= player.direction.x * MOVE_SPEED * deltaTime;
        
        if (canMoveTo(player, newPos, map)) {
            player.position = newPos;
        }
    }
    
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
        sf::Vector2f newPos = player.position;
        newPos.x -= player.direction.y * MOVE_SPEED * deltaTime;
        newPos.y += player.direction.x * MOVE_SPEED * deltaTime;
        
        if (canMoveTo(player, newPos, map)) {
            player.position = newPos;
        }
    }
    
    // Вращение (без коллизий)
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
        float oldDirX = player.direction.x;
        player.direction.x = player.direction.x * cos(ROTATION_SPEED * deltaTime) - 
                            player.direction.y * sin(ROTATION_SPEED * deltaTime);
        player.direction.y = oldDirX * sin(ROTATION_SPEED * deltaTime) + 
                            player.direction.y * cos(ROTATION_SPEED * deltaTime);
        
        float oldPlaneX = player.plane.x;
        player.plane.x = player.plane.x * cos(ROTATION_SPEED * deltaTime) - 
                        player.plane.y * sin(ROTATION_SPEED * deltaTime);
        player.plane.y = oldPlaneX * sin(ROTATION_SPEED * deltaTime) + 
                        player.plane.y * cos(ROTATION_SPEED * deltaTime);
    }
    
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
        float oldDirX = player.direction.x;
        player.direction.x = player.direction.x * cos(-ROTATION_SPEED * deltaTime) - 
                            player.direction.y * sin(-ROTATION_SPEED * deltaTime);
        player.direction.y = oldDirX * sin(-ROTATION_SPEED * deltaTime) + 
                            player.direction.y * cos(-ROTATION_SPEED * deltaTime);
        
        float oldPlaneX = player.plane.x;
        player.plane.x = player.plane.x * cos(-ROTATION_SPEED * deltaTime) - 
                        player.plane.y * sin(-ROTATION_SPEED * deltaTime);
        player.plane.y = oldPlaneX * sin(-ROTATION_SPEED * deltaTime) + 
                        player.plane.y * cos(-ROTATION_SPEED * deltaTime);
    }
}

bool InputHandler::canMoveTo(const PlayerState& player, const sf::Vector2f& newPos, 
                            const std::vector<std::vector<int>>& map) const {
    // Проверяем, не находится ли новая позиция внутри стены
    int mapX = static_cast<int>(newPos.x);
    int mapY = static_cast<int>(newPos.y);
    
    // Если вышли за границы карты - запрещаем движение
    if (mapX < 0 || mapY < 0 || mapY >= map.size() || mapX >= map[0].size()) {
        return false;
    }
    
    // Проверяем, является ли клетка стеной
    return map[mapY][mapX] == 0;
}