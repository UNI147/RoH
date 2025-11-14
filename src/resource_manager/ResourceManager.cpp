#include "ResourceManager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include "resource_manager/TextureNames.h"
#include "resource_manager/MusicNames.h"

namespace fs = std::filesystem;

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

bool ResourceManager::loadSound(const std::string& name, const std::string& filename) {
    auto soundBuffer = std::make_unique<sf::SoundBuffer>();
    
    if (!soundBuffer->loadFromFile(filename)) {
        std::cerr << "Failed to load sound: " << filename << std::endl;
        return false;
    }
    
    soundBuffers_[name] = std::move(soundBuffer);
    std::cout << "Loaded sound: " << name << " from " << filename << std::endl;
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
    // Проверяем существование файла
    FILE* testFile = nullptr;
    errno_t err = fopen_s(&testFile, filename.c_str(), "rb");
    if (err != 0 || !testFile) {
        std::cerr << "MIDI file not found: " << filename << " (error: " << err << ")" << std::endl;
        return false;
    }
    fclose(testFile);
    
    midiFiles_[name] = filename;
    std::cout << "Loaded MIDI: " << name << " from " << filename << std::endl;
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
    // Проверяем существование файла
    FILE* testFile = nullptr;
    errno_t err = fopen_s(&testFile, filename.c_str(), "rb");
    if (err != 0 || !testFile) {
        std::cerr << "SoundFont file not found: " << filename << " (error: " << err << ")" << std::endl;
        return false;
    }
    fclose(testFile);
    
    soundFonts_[name] = filename;
    std::cout << "Loaded SoundFont: " << name << " from " << filename << std::endl;
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
    auto font = std::make_unique<sf::Font>();
    
    if (!font->loadFromFile(filename)) {
        std::cerr << "Failed to load font: " << filename << std::endl;
        return false;
    }
    
    fonts_[name] = std::move(font);
    std::cout << "Loaded font: " << name << " from " << filename << std::endl;
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

bool ResourceManager::loadLevel(const std::string& name, const std::string& filename) {
    // Проверяем существование файла
    FILE* testFile = nullptr;
    errno_t err = fopen_s(&testFile, filename.c_str(), "rb");
    if (err != 0 || !testFile) {
        std::cerr << "Level file not found: " << filename << " (error: " << err << ")" << std::endl;
        return false;
    }
    fclose(testFile);
    
    auto levelData = std::make_unique<LevelData>();
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        std::cerr << "Failed to load level file: " << filename << std::endl;
        *levelData = createTestLevel();
    } else {
        std::string line;
        bool readingGrid = false;
        std::vector<std::vector<int>> grid;
        
        while (std::getline(file, line)) {
            // Пропускаем пустые строки и комментарии
            if (line.empty() || line[0] == '#') continue;
            
            // Обрабатываем секции
            if (line == "[GRID]") {
                readingGrid = true;
                continue;
            } else if (line == "[RESOURCES]") {
                readingGrid = false;
                continue;
            } else if (line == "[PLAYER]") {
                readingGrid = false;
                continue;
            }
            
            if (readingGrid) {
                // Чтение сетки уровня
                std::vector<int> row;
                std::istringstream iss(line);
                int value;
                
                while (iss >> value) {
                    row.push_back(value);
                }
                
                if (!row.empty()) {
                    grid.push_back(row);
                }
            } else {
                // Обработка ресурсов и позиции игрока
                if (!parseResourceLine(line, *levelData) && !parsePlayerPosition(line, *levelData)) {
                    std::cout << "Unknown level directive: " << line << std::endl;
                }
            }
        }
        
        levelData->grid = grid;
        file.close();
    }
    
    levels_[name] = std::move(levelData);
    std::cout << "Loaded level: " << name << " from " << filename << std::endl;
    
    // Загружаем текстуры сразу после загрузки уровня
    for (const auto& [texName, texPath] : levels_[name]->textures) {
        if (!loadTexture(texName, texPath)) {
            std::cerr << "Failed to load level texture: " << texName << " from " << texPath << std::endl;
        }
    }
    
    return true;
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

bool ResourceManager::parseResourceLine(const std::string& line, LevelData& level) {
    std::istringstream iss(line);
    std::string type, name, path;
    
    if (!(iss >> type >> name >> path)) {
        return false;
    }
    
    if (type == "texture") {
        level.textures[name] = path;
        return true;
    } else if (type == "music") {
        level.backgroundMusic = path;
        return true;
    }
    
    return false;
}

bool ResourceManager::parsePlayerPosition(const std::string& line, LevelData& level) {
    std::istringstream iss(line);
    std::string type;
    float x, y, dirX, dirY;
    
    if (iss >> type >> x >> y >> dirX >> dirY && type == "player") {
        level.playerStartPosition = sf::Vector2f(x, y);
        level.playerStartDirection = sf::Vector2f(dirX, dirY);
        return true;
    }
    
    return false;
}

LevelData ResourceManager::createTestLevel() {
    LevelData level;
    
    // Тестовая сетка
    level.grid = {
        {1, 1, 1, 1, 1, 1, 1, 1},
        {1, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 1},
        {1, 1, 1, 1, 1, 1, 1, 1}
    };
    
    // Пути к ресурсам для тестового уровня
    std::vector<std::string> possibleTexturePaths = {
        "resources/textures/surfaces/",
        "../resources/textures/surfaces/", 
        "../../resources/textures/surfaces/",
        "../../../resources/textures/surfaces/",
        "textures/surfaces/",
        "../textures/surfaces/"
    };
    
    std::vector<std::string> possibleMusicPaths = {
        "resources/sounds/music/",
        "../resources/sounds/music/",
        "../../resources/sounds/music/", 
        "../../../resources/sounds/music/",
        "sounds/music/",
        "../sounds/music/"
    };
    
    // Поиск текстур
    for (const auto& path : possibleTexturePaths) {
        std::string bricksPath = path + "bricks.png";
        if (fs::exists(bricksPath)) {
            level.textures[Textures::BRICKS] = bricksPath;
            level.textures[Textures::BOARDS] = path + "boards.png";
            level.textures[Textures::PARQUET] = path + "parquet.png";
            break;
        }
    }
    
    // Поиск музыки
    for (const auto& path : possibleMusicPaths) {
        std::vector<std::string> possibleMusicFiles = {
            "Adrian'sAsleep.mid"
        };
        
        for (const auto& musicFile : possibleMusicFiles) {
            std::string musicPath = path + musicFile;
            if (fs::exists(musicPath)) {
                level.backgroundMusic = musicPath;
                std::cout << "Found music file: " << musicPath << std::endl;
                break;
            }
        }
        
        if (!level.backgroundMusic.empty()) {
            break;
        }
    }
    
    // Стартовая позиция игрока
    level.playerStartPosition = sf::Vector2f(1.5f, 1.5f);
    level.playerStartDirection = sf::Vector2f(-1.0f, 0.0f);
    
    return level;
}

void ResourceManager::clear() {
    textures_.clear();
    musicTracks_.clear();
    soundBuffers_.clear();
    midiFiles_.clear();
    soundFonts_.clear();
    levels_.clear();
}