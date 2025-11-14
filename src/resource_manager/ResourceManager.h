#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

// Добавляем структуру уровня
struct LevelData {
    std::vector<std::vector<int>> grid;
    std::unordered_map<std::string, std::string> textures;
    std::string backgroundMusic;
    sf::Vector2f playerStartPosition;
    sf::Vector2f playerStartDirection;
};

class ResourceManager {
public:
    static ResourceManager& getInstance();
    
    // Загрузка текстур
    bool loadTexture(const std::string& name, const std::string& filename);
    sf::Texture& getTexture(const std::string& name);
    bool hasTexture(const std::string& name) const;
    
    // Загрузка шрифтов
    bool loadFont(const std::string& name, const std::string& filename);
    sf::Font& getFont(const std::string& name);
    bool hasFont(const std::string& name) const;
    
    // Загрузка музыки
    bool loadMusic(const std::string& name, const std::string& filename);
    sf::Music& getMusic(const std::string& name);
    bool hasMusic(const std::string& name) const;
    
    // Загрузка звуковых эффектов
    bool loadSound(const std::string& name, const std::string& filename);
    sf::SoundBuffer& getSound(const std::string& name);
    bool hasSound(const std::string& name) const;
    
    // Загрузка MIDI файлов
    bool loadMIDI(const std::string& name, const std::string& filename);
    const std::string& getMIDIPath(const std::string& name) const;
    bool hasMIDI(const std::string& name) const;
    
    // Загрузка SoundFont
    bool loadSoundFont(const std::string& name, const std::string& filename);
    const std::string& getSoundFontPath(const std::string& name) const;
    bool hasSoundFont(const std::string& name) const;
    
    // Загрузка уровней
    bool loadLevel(const std::string& name, const std::string& filename);
    const LevelData& getLevel(const std::string& name) const;
    bool hasLevel(const std::string& name) const;
    
    // Создание тестового уровня
    LevelData createTestLevel();
    
    // Очистка всех ресурсов
    void clear();

private:
    ResourceManager() = default;
    ~ResourceManager() = default;
    
    std::unordered_map<std::string, std::unique_ptr<sf::Texture>> textures_;
    std::unordered_map<std::string, std::unique_ptr<sf::Font>> fonts_;
    std::unordered_map<std::string, std::unique_ptr<sf::Music>> musicTracks_;
    std::unordered_map<std::string, std::unique_ptr<sf::SoundBuffer>> soundBuffers_;
    
    // MIDI и SoundFont хранятся как пути к файлам
    std::unordered_map<std::string, std::string> midiFiles_;
    std::unordered_map<std::string, std::string> soundFonts_;
    
    // Уровни
    std::unordered_map<std::string, std::unique_ptr<LevelData>> levels_;
    
    // Вспомогательные методы для парсинга уровня
    bool parseResourceLine(const std::string& line, LevelData& level);
    bool parsePlayerPosition(const std::string& line, LevelData& level);
    
    // Запрещаем копирование
    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;
};