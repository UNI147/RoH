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

bool ResourceManager::loadMusic(const std::string& name, const std::string& filename) {
    auto music = std::make_unique<sf::Music>();
    
    if (!music->openFromFile(filename)) {
        std::cerr << "Failed to load music: " << filename << std::endl;
        return false;
    }
    
    musicTracks_[name] = std::move(music);
    std::cout << "Loaded music: " << name << " from " << filename << std::endl;
    return true;
}

sf::Music& ResourceManager::getMusic(const std::string& name) {
    auto it = musicTracks_.find(name);
    if (it != musicTracks_.end()) {
        return *it->second;
    }
    
    throw std::runtime_error("Music not found: " + name);
}

bool ResourceManager::hasMusic(const std::string& name) const {
    return musicTracks_.find(name) != musicTracks_.end();
}

void ResourceManager::clear() {
    textures_.clear();
    musicTracks_.clear();
}