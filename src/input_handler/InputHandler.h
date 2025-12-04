#pragma once
#include <SFML/Window/Keyboard.hpp>
#include "../engine/raycasting/RayCaster.h"
#include <vector>
#include "../geographer/Furniture.h"

class InputHandler {
public:
    void handleInput(PlayerState& player, float deltaTime, 
                     const std::vector<std::vector<int>>& map,
                     const std::vector<FurnitureObject>& furniture,
                     const std::unordered_map<std::string, FurnitureData>& furnitureTypes);
    bool isRunning() const { return isRunning_; }

private:
    const float MOVE_SPEED = 3.0f;
    const float RUN_SPEED = 6.0f;
    const float ROTATION_SPEED = 2.0f;
    
    bool isRunning_ = false;
    
    bool canMoveTo(const PlayerState& player, const sf::Vector2f& newPos, 
                  const std::vector<std::vector<int>>& map,
                  const std::vector<FurnitureObject>& furniture,
                  const std::unordered_map<std::string, FurnitureData>& furnitureTypes) const;
    
    float getCurrentMoveSpeed() const;
};