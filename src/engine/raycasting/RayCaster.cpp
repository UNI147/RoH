#include "RayCaster.h"
#include <cmath>
#include <algorithm>

RayHit RayCaster::castRay(const PlayerState& player, const std::vector<std::vector<int>>& map, 
                         float cameraX) const {
    RayHit hit;
    
    // Вычисление направления луча
    hit.rayDirX = player.direction.x + player.plane.x * cameraX;
    hit.rayDirY = player.direction.y + player.plane.y * cameraX;
    
    // Позиция игрока на карте
    int mapX = static_cast<int>(player.position.x);
    int mapY = static_cast<int>(player.position.y);
    
    // Длина луча от текущей позиции до следующей x или y-стороны
    float deltaDistX = std::abs(1 / hit.rayDirX);
    float deltaDistY = std::abs(1 / hit.rayDirY);
    
    // Направление шага и начальное расстояние до стороны
    int stepX, stepY;
    float sideDistX, sideDistY;
    
    if (hit.rayDirX < 0) {
        stepX = -1;
        sideDistX = (player.position.x - mapX) * deltaDistX;
    } else {
        stepX = 1;
        sideDistX = (mapX + 1.0f - player.position.x) * deltaDistX;
    }
    
    if (hit.rayDirY < 0) {
        stepY = -1;
        sideDistY = (player.position.y - mapY) * deltaDistY;
    } else {
        stepY = 1;
        sideDistY = (mapY + 1.0f - player.position.y) * deltaDistY;
    }
    
    // Алгоритм DDA
    bool hitDetected = false;
    int side; // 0 = x-side, 1 = y-side
    
    while (!hitDetected) {
        // Переход к следующему квадрату карты
        if (sideDistX < sideDistY) {
            sideDistX += deltaDistX;
            mapX += stepX;
            side = 0;
        } else {
            sideDistY += deltaDistY;
            mapY += stepY;
            side = 1;
        }
        
        // Проверка, является ли квадрат стеной
        if (isWall(map, mapX, mapY)) {
            hitDetected = true;
        }
    }
    
    // Вычисление расстояния до стены
    if (side == 0) {
        hit.distance = (mapX - player.position.x + (1 - stepX) / 2) / hit.rayDirX;
    } else {
        hit.distance = (mapY - player.position.y + (1 - stepY) / 2) / hit.rayDirY;
    }
    
    hit.mapX = mapX;
    hit.mapY = mapY;
    hit.side = side;
    
    // Вычисление позиции удара о стену для текстурирования
    if (side == 0) {
        hit.wallX = player.position.y + hit.distance * hit.rayDirY;
    } else {
        hit.wallX = player.position.x + hit.distance * hit.rayDirX;
    }
    hit.wallX -= std::floor(hit.wallX);
    
    // Вычисление координат пола для текстурирования
    if (side == 0 && hit.rayDirX > 0) {
        hit.floorXWall = static_cast<float>(mapX);
        hit.floorYWall = static_cast<float>(mapY) + hit.wallX;
    } else if (side == 0 && hit.rayDirX < 0) {
        hit.floorXWall = static_cast<float>(mapX) + 1.0f;
        hit.floorYWall = static_cast<float>(mapY) + hit.wallX;
    } else if (side == 1 && hit.rayDirY > 0) {
        hit.floorXWall = static_cast<float>(mapX) + hit.wallX;
        hit.floorYWall = static_cast<float>(mapY);
    } else {
        hit.floorXWall = static_cast<float>(mapX) + hit.wallX;
        hit.floorYWall = static_cast<float>(mapY) + 1.0f;
    }
    
    if (hitDetected) {
        // Получаем ID текстуры из карты
        hit.textureId = map[mapY][mapX];
    }
    
    return hit;
}

bool RayCaster::isWall(const std::vector<std::vector<int>>& map, int x, int y) const {
    if (x < 0 || y < 0 || y >= map.size() || x >= map[0].size()) {
        return true; // За пределами карты = стена
    }
    return map[y][x] != 0;
}