#include "InputHandler.h"
#include <cmath>

void InputHandler::handleInput(PlayerState& player, float deltaTime, 
                              const std::vector<std::vector<int>>& map) {
    // Сохраняем старую позицию для отката при коллизии
    sf::Vector2f oldPosition = player.position;
    
    // Определяем, бежит ли игрок
    isRunning_ = (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) || 
                 sf::Keyboard::isKeyPressed(sf::Keyboard::RShift));
    
    float currentMoveSpeed = getCurrentMoveSpeed() * deltaTime;
    float currentRotationSpeed = ROTATION_SPEED * deltaTime;
    
    // Движение вперед (Стрелка вверх/W)
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up) || sf::Keyboard::isKeyPressed(sf::Keyboard::W)) {
        sf::Vector2f newPos = player.position;
        newPos.x += player.direction.x * currentMoveSpeed;
        newPos.y += player.direction.y * currentMoveSpeed;
        
        if (canMoveTo(player, newPos, map)) {
            player.position = newPos;
        }
    }
    
    // Движение назад (Стрелка вниз/S)
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down) || sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
        sf::Vector2f newPos = player.position;
        newPos.x -= player.direction.x * currentMoveSpeed;
        newPos.y -= player.direction.y * currentMoveSpeed;
        
        if (canMoveTo(player, newPos, map)) {
            player.position = newPos;
        }
    }
    
    // Стрэйф влево (Запятая/A)
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Comma) || sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
        sf::Vector2f newPos = player.position;
        newPos.x -= player.direction.y * currentMoveSpeed;
        newPos.y += player.direction.x * currentMoveSpeed;
        
        if (canMoveTo(player, newPos, map)) {
            player.position = newPos;
        }
    }

    // Стрэйф вправо (Точка/D)
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Period) || sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
        sf::Vector2f newPos = player.position;
        newPos.x += player.direction.y * currentMoveSpeed;
        newPos.y -= player.direction.x * currentMoveSpeed;
        
        if (canMoveTo(player, newPos, map)) {
            player.position = newPos;
        }
    }
    
    // Поворот налево (Стрелка влево/Q)
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left) || sf::Keyboard::isKeyPressed(sf::Keyboard::Q)) {
        float oldDirX = player.direction.x;
        player.direction.x = player.direction.x * cos(currentRotationSpeed) - 
                            player.direction.y * sin(currentRotationSpeed);
        player.direction.y = oldDirX * sin(currentRotationSpeed) + 
                            player.direction.y * cos(currentRotationSpeed);
        
        float oldPlaneX = player.plane.x;
        player.plane.x = player.plane.x * cos(currentRotationSpeed) - 
                        player.plane.y * sin(currentRotationSpeed);
        player.plane.y = oldPlaneX * sin(currentRotationSpeed) + 
                        player.plane.y * cos(currentRotationSpeed);
    }
    
    // Поворот направо (Стрелка вправо/E)
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right) || sf::Keyboard::isKeyPressed(sf::Keyboard::E)) {
        float oldDirX = player.direction.x;
        player.direction.x = player.direction.x * cos(-currentRotationSpeed) - 
                            player.direction.y * sin(-currentRotationSpeed);
        player.direction.y = oldDirX * sin(-currentRotationSpeed) + 
                            player.direction.y * cos(-currentRotationSpeed);
        
        float oldPlaneX = player.plane.x;
        player.plane.x = player.plane.x * cos(-currentRotationSpeed) - 
                        player.plane.y * sin(-currentRotationSpeed);
        player.plane.y = oldPlaneX * sin(-currentRotationSpeed) + 
                        player.plane.y * cos(-currentRotationSpeed);
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

float InputHandler::getCurrentMoveSpeed() const {
    return isRunning_ ? RUN_SPEED : MOVE_SPEED;
}