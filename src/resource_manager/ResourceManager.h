#pragma once
#include <SFML/Graphics.hpp>
#include <unordered_map>
#include <string>
#include <memory>

class ResourceManager {
public:
    static ResourceManager& getInstance();
    
    // Загрузка текстуры
    bool loadTexture(const std::string& name, const std::string& filename);
    
    // Получение текстуры
    sf::Texture& getTexture(const std::string& name);
    
    // Проверка существования текстуры
    bool hasTexture(const std::string& name) const;
    
    // Очистка всех ресурсов
    void clear();

private:
    ResourceManager() = default;
    ~ResourceManager() = default;
    
    std::unordered_map<std::string, std::unique_ptr<sf::Texture>> textures_;
    
    // Запрещаем копирование
    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;
};