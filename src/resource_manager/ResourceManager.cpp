#include "ResourceManager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

ResourceManager& ResourceManager::getInstance() {
    static ResourceManager instance;
    return instance;
}

// Метод для поиска файла по альтернативным путям
std::string ResourceManager::findResourceFile(const std::string& filename) const {
    FILE* testFile = nullptr;
    
    std::vector<std::string> possiblePaths = {
        filename,
        basePath_ + filename,
        "../" + basePath_ + filename,
        "../../" + basePath_ + filename,
        "../../../" + basePath_ + filename,
        "resources/" + filename,
        "../resources/" + filename,
        "../../resources/" + filename
    };
    
    for (const auto& path : possiblePaths) {
        if (fopen_s(&testFile, path.c_str(), "rb") == 0 && testFile) {
            fclose(testFile);
            std::cout << "Found resource at: " << path << std::endl;
            return path;
        }
    }
    
    return "";
}

// Обновляем методы загрузки
bool ResourceManager::loadTexture(const std::string& name, const std::string& filename) {
    std::string foundPath = findResourceFile(filename);
    if (foundPath.empty()) {
        std::cerr << "Texture file not found: " << filename << std::endl;
        return false;
    }
    
    auto texture = std::make_unique<sf::Texture>();
    if (!texture->loadFromFile(foundPath)) {
        std::cerr << "Failed to load texture: " << foundPath << std::endl;
        return false;
    }
    
    texture->setSmooth(false);
    texture->setRepeated(true);
    textures_[name] = std::move(texture);
    std::cout << "Loaded texture: " << name << " from " << foundPath << std::endl;
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
    std::string foundPath = findResourceFile(filename);
    if (foundPath.empty()) {
        std::cerr << "Music file not found: " << filename << std::endl;
        return false;
    }
    
    auto music = std::make_unique<sf::Music>();
    
    if (!music->openFromFile(foundPath)) {
        std::cerr << "Failed to load music: " << foundPath << std::endl;
        return false;
    }
    
    musicTracks_[name] = std::move(music);
    std::cout << "Loaded music: " << name << " from " << foundPath << std::endl;
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

bool ResourceManager::loadSound(const std::string& name, const std::string& filename) {
    std::string foundPath = findResourceFile(filename);
    if (foundPath.empty()) {
        std::cerr << "Sound file not found: " << filename << std::endl;
        return false;
    }
    
    auto soundBuffer = std::make_unique<sf::SoundBuffer>();
    if (!soundBuffer->loadFromFile(foundPath)) {
        std::cerr << "Failed to load sound: " << foundPath << std::endl;
        return false;
    }
    
    soundBuffers_[name] = std::move(soundBuffer);
    std::cout << "Loaded sound: " << name << " from " << foundPath << std::endl;
    return true;
}

sf::SoundBuffer& ResourceManager::getSound(const std::string& name) {
    auto it = soundBuffers_.find(name);
    if (it != soundBuffers_.end()) {
        return *it->second;
    }
    
    throw std::runtime_error("Sound not found: " + name);
}

bool ResourceManager::hasSound(const std::string& name) const {
    return soundBuffers_.find(name) != soundBuffers_.end();
}

bool ResourceManager::loadMIDI(const std::string& name, const std::string& filename) {
    std::string foundPath = findResourceFile(filename);
    if (foundPath.empty()) {
        std::cerr << "MIDI file not found: " << filename << std::endl;
        return false;
    }
    
    midiFiles_[name] = foundPath;
    std::cout << "Loaded MIDI: " << name << " from " << foundPath << std::endl;
    return true;
}

const std::string& ResourceManager::getMIDIPath(const std::string& name) const {
    auto it = midiFiles_.find(name);
    if (it != midiFiles_.end()) {
        return it->second;
    }
    
    throw std::runtime_error("MIDI not found: " + name);
}

bool ResourceManager::hasMIDI(const std::string& name) const {
    return midiFiles_.find(name) != midiFiles_.end();
}

bool ResourceManager::loadSoundFont(const std::string& name, const std::string& filename) {
    std::string foundPath = findResourceFile(filename);
    if (foundPath.empty()) {
        std::cerr << "SoundFont file not found: " << filename << std::endl;
        return false;
    }
    
    soundFonts_[name] = foundPath;
    std::cout << "Loaded SoundFont: " << name << " from " << foundPath << std::endl;
    return true;
}

const std::string& ResourceManager::getSoundFontPath(const std::string& name) const {
    auto it = soundFonts_.find(name);
    if (it != soundFonts_.end()) {
        return it->second;
    }
    
    throw std::runtime_error("SoundFont not found: " + name);
}

bool ResourceManager::hasSoundFont(const std::string& name) const {
    return soundFonts_.find(name) != soundFonts_.end();
}

bool ResourceManager::loadFont(const std::string& name, const std::string& filename) {
    std::string foundPath = findResourceFile(filename);
    if (foundPath.empty()) {
        std::cerr << "Font file not found: " << filename << std::endl;
        return false;
    }
    
    auto font = std::make_unique<sf::Font>();
    
    if (!font->loadFromFile(foundPath)) {
        std::cerr << "Failed to load font: " << foundPath << std::endl;
        return false;
    }
    
    fonts_[name] = std::move(font);
    std::cout << "Loaded font: " << name << " from " << foundPath << std::endl;
    return true;
}

sf::Font& ResourceManager::getFont(const std::string& name) {
    auto it = fonts_.find(name);
    if (it != fonts_.end()) {
        return *it->second;
    }
    
    throw std::runtime_error("Font not found: " + name);
}

bool ResourceManager::hasFont(const std::string& name) const {
    return fonts_.find(name) != fonts_.end();
}

// Хранение уровней
void ResourceManager::addLevel(const std::string& name, std::unique_ptr<LevelData> level) {
    levels_[name] = std::move(level);
    std::cout << "Level added to ResourceManager: " << name << std::endl;
}

const LevelData& ResourceManager::getLevel(const std::string& name) const {
    auto it = levels_.find(name);
    if (it != levels_.end()) {
        return *it->second;
    }
    
    throw std::runtime_error("Level not found: " + name);
}

bool ResourceManager::hasLevel(const std::string& name) const {
    return levels_.find(name) != levels_.end();
}

// Создание тестового уровня
std::unique_ptr<LevelData> ResourceManager::createTestLevel() {
    auto level = std::make_unique<LevelData>();
    
    // Тестовые сетки 8x8
    level->wallGrid = {
        {1, 1, 1, 1, 1, 1, 1, 1},
        {1, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 1, 0, 0, 1, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 1, 0, 0, 1, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 1},
        {1, 1, 1, 1, 1, 1, 1, 1}
    };
    
    // Тестовая сетка пола (все 1 - одна текстура)
    level->floorGrid = std::vector<std::vector<int>>(8, std::vector<int>(8, 1));
    
    // Тестовая сетка потолка (все 1 - одна текстура)
    level->ceilingGrid = std::vector<std::vector<int>>(8, std::vector<int>(8, 1));
    
    // Текстуры стен
    level->wallTextures[0] = "wall_0";
    level->wallTextures[1] = "wall_1";
    
    // Текстуры пола
    level->floorTextures[1] = "floor_1";
    
    // Текстуры потолка
    level->ceilingTextures[1] = "ceiling_1";
    
    // Пути к текстурам
    level->texturePaths["wall_0"] = "textures/surfaces/stone.png";
    level->texturePaths["wall_1"] = "textures/surfaces/bricks.png";
    level->texturePaths["floor_1"] = "textures/surfaces/parquet.png";
    level->texturePaths["ceiling_1"] = "textures/surfaces/boards.png";
    
    // Музыка и звуки
    level->backgroundMusic = "sounds/music/Adrian'sAsleep.mid";
    level->ambienceSound = "sounds/effects/ambienceloop.wav";
    
    // Стартовая позиция игрока
    level->playerStartPosition = sf::Vector2f(1.5f, 1.5f);
    level->playerStartDirection = sf::Vector2f(-1.0f, 0.0f);
    
    level->name = "Test Level with Three Layers";
    
    return level;
}

// Метод для массовой загрузки
bool ResourceManager::loadResourceBatch(const std::vector<std::pair<std::string, std::string>>& resources) {
    bool allLoaded = true;
    
    for (const auto& [name, path] : resources) {
        bool loaded = false;
        
        // Определяем тип ресурса по расширению файла
        std::string extension = getFileExtension(path);
        
        if (extension == "ttf" || extension == "otf") {
            // Шрифты
            loaded = loadFont(name, path);
        } 
        else if (extension == "wav" || extension == "ogg" || extension == "flac") {
            // Звуки
            loaded = loadSound(name, path);
        }
        else if (extension == "png" || extension == "jpg" || extension == "jpeg" || extension == "bmp") {
            // Текстуры
            loaded = loadTexture(name, path);
        }
        else if (extension == "mid" || extension == "midi") {
            // MIDI файлы
            loaded = loadMIDI(name, path);
        }
        else if (extension == "sf2") {
            // SoundFont
            loaded = loadSoundFont(name, path);
        }
        else {
            // Пробуем определить по содержимому
            if (!loadTexture(name, path) && !loadSound(name, path) && !loadMusic(name, path) && !loadFont(name, path)) {
                std::cerr << "Failed to load resource (unknown type): " << name << " from " << path << std::endl;
                allLoaded = false;
            } else {
                loaded = true;
            }
        }
        
        if (!loaded) {
            std::cerr << "Failed to load resource: " << name << " from " << path << std::endl;
            allLoaded = false;
        }
    }
    
    return allLoaded;
}

// Вспомогательный метод для получения расширения файла
std::string ResourceManager::getFileExtension(const std::string& filename) const {
    size_t dotPos = filename.find_last_of(".");
    if (dotPos != std::string::npos) {
        std::string ext = filename.substr(dotPos + 1);
        // Приводим к нижнему регистру для сравнения
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        return ext;
    }
    return "";
}

void ResourceManager::clear() {
    textures_.clear();
    musicTracks_.clear();
    soundBuffers_.clear();
    midiFiles_.clear();
    soundFonts_.clear();
    levels_.clear();
}