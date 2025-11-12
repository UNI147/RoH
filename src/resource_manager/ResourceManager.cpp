#include "ResourceManager.h"
#include <iostream>

ResourceManager& ResourceManager::getInstance() {
    static ResourceManager instance;
    return instance;
}

bool ResourceManager::loadTexture(const std::string& name, const std::string& filename) {
    auto texture = std::make_unique<sf::Texture>();
    
    if (!texture->loadFromFile(filename)) {
        std::cerr << "Failed to load texture: " << filename << std::endl;
        return false;
    }
    
    // Отключаем сглаживание для пиксель-арт стиля
    texture->setSmooth(false);
    texture->setRepeated(true);
    
    textures_[name] = std::move(texture);
    std::cout << "Loaded texture: " << name << " from " << filename << std::endl;
    return true;
}

sf::Texture& ResourceManager::getTexture(const std::string& name) {
    auto it = textures_.find(name);
    if (it != textures_.end()) {
        return *it->second;
    }
    
    throw std::runtime_error("Texture not found: " + name);
}

bool ResourceManager::hasTexture(const std::string& name) const {
    return textures_.find(name) != textures_.end();
}

void ResourceManager::clear() {
    textures_.clear();
}