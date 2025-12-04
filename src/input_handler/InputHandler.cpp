#include "InputHandler.h"
#include <cmath>

void InputHandler::handleInput(PlayerState& player, float deltaTime, 
                              const std::vector<std::vector<int>>& map,
                              const std::vector<FurnitureObject>& furniture,
                              const std::unordered_map<std::string, FurnitureData>& furnitureTypes) {
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
        
        if (canMoveTo(player, newPos, map, furniture, furnitureTypes)) {
            player.position = newPos;
        }
    }
    
    // Движение назад (Стрелка вниз/S)
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down) || sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
        sf::Vector2f newPos = player.position;
        newPos.x -= player.direction.x * currentMoveSpeed;
        newPos.y -= player.direction.y * currentMoveSpeed;
        
        if (canMoveTo(player, newPos, map, furniture, furnitureTypes)) {
            player.position = newPos;
        }
    }
    
    // Стрэйф влево (Запятая/A)
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Comma) || sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
        sf::Vector2f newPos = player.position;
        newPos.x -= player.direction.y * currentMoveSpeed;
        newPos.y += player.direction.x * currentMoveSpeed;
        
        if (canMoveTo(player, newPos, map, furniture, furnitureTypes)) {
            player.position = newPos;
        }
    }

    // Стрэйф вправо (Точка/D)
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Period) || sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
        sf::Vector2f newPos = player.position;
        newPos.x += player.direction.y * currentMoveSpeed;
        newPos.y -= player.direction.x * currentMoveSpeed;
        
        if (canMoveTo(player, newPos, map, furniture, furnitureTypes)) {
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
                            const std::vector<std::vector<int>>& map,
                            const std::vector<FurnitureObject>& furniture,
                            const std::unordered_map<std::string, FurnitureData>& furnitureTypes) const {
    
    // 1. Проверяем стены
    int mapX = static_cast<int>(newPos.x);
    int mapY = static_cast<int>(newPos.y);
    
    if (mapX < 0 || mapY < 0 || mapY >= map.size() || mapX >= map[0].size()) {
        return false;
    }
    
    if (map[mapY][mapX] != 0) {
        return false;
    }
    
    // 2. Проверяем столкновения с мебелью
    for (const auto& obj : furniture) {
        auto it = furnitureTypes.find(obj.furnitureType);
        if (it != furnitureTypes.end() && !it->second.passable) {
            const auto& data = it->second;
            
            // Вычисляем мировые координаты коллизионной коробки
            float collisionX = obj.position.x + data.collisionBox.x - data.collisionBox.width/2;
            float collisionY = obj.position.y + data.collisionBox.y - data.collisionBox.height/2;
            float collisionWidth = data.collisionBox.width;
            float collisionHeight = data.collisionBox.height;
            
            // Проверяем коллизию с новой позицией игрока
            float playerRadius = 0.2f;
            
            // Проверяем пересечение окружности (игрок) с прямоугольником (мебель)
            float closestX = std::max(collisionX, std::min(newPos.x, collisionX + collisionWidth));
            float closestY = std::max(collisionY, std::min(newPos.y, collisionY + collisionHeight));
            
            float distanceX = newPos.x - closestX;
            float distanceY = newPos.y - closestY;
            float distanceSquared = distanceX * distanceX + distanceY * distanceY;
            
            if (distanceSquared < (playerRadius * playerRadius)) {
                return false;
            }
        }
    }
    
    return true;
}

float InputHandler::getCurrentMoveSpeed() const {
    return isRunning_ ? RUN_SPEED : MOVE_SPEED;
}