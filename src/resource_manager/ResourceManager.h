#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <unordered_map>
#include <string>
#include <memory>

class ResourceManager {
public:
    static ResourceManager& getInstance();
    
    // Загрузка текстур
    bool loadTexture(const std::string& name, const std::string& filename);
    sf::Texture& getTexture(const std::string& name);
    bool hasTexture(const std::string& name) const;
    
    // Загрузка музыки
    bool loadMusic(const std::string& name, const std::string& filename);
    sf::Music& getMusic(const std::string& name);
    bool hasMusic(const std::string& name) const;
    
    // Очистка всех ресурсов
    void clear();

private:
    ResourceManager() = default;
    ~ResourceManager() = default;
    
    std::unordered_map<std::string, std::unique_ptr<sf::Texture>> textures_;
    std::unordered_map<std::string, std::unique_ptr<sf::Music>> musicTracks_;
    
    // Запрещаем копирование
    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;
};