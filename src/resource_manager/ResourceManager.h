#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

// Структура уровня
struct LevelData {
    std::string name;
    std::vector<std::vector<int>> wallGrid;
    std::vector<std::vector<int>> floorGrid;
    std::vector<std::vector<int>> ceilingGrid;
    
    // Текстуры по типам
    std::unordered_map<int, std::string> wallTextures;
    std::unordered_map<int, std::string> floorTextures;
    std::unordered_map<int, std::string> ceilingTextures;
    
    std::string backgroundMusic;
    std::string ambienceSound;
    sf::Vector2f playerStartPosition;
    sf::Vector2f playerStartDirection;
    
    // Сопоставление имен текстур с путями (для загрузки)
    std::unordered_map<std::string, std::string> texturePaths;
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
    
    // Хранение уровней (загруженных через MapLoader)
    void addLevel(const std::string& name, std::unique_ptr<LevelData> level);
    const LevelData& getLevel(const std::string& name) const;
    bool hasLevel(const std::string& name) const;
    
    // Создание тестового уровня
    std::unique_ptr<LevelData> createTestLevel();

    std::string findResourceFile(const std::string& filename) const;
    bool loadResourceBatch(const std::vector<std::pair<std::string, std::string>>& resources);
    
    void setBasePath(const std::string& path) { basePath_ = path; }
    std::string getFullPath(const std::string& relativePath) const {
        return basePath_ + relativePath;
    }
    
    // Утилиты
    std::string getFileExtension(const std::string& filename) const;
        
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
    
    // Уровни (только хранение)
    std::unordered_map<std::string, std::unique_ptr<LevelData>> levels_;
    
    std::string basePath_ = "resources/";
    
    // Запрещаем копирование
    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;
};