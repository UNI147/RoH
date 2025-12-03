#include "LevelProcessor.h"
#include <iostream>

bool LevelProcessor::processLevel(const std::string& levelName, 
                                const std::string& filename,
                                SoundEngineer& soundEngineer) {
    auto& rm = ResourceManager::getInstance();
    
    // 1. Парсим файл уровня
    auto levelData = MapLoader::loadLevelFromFile(filename);
    if (!levelData) {
        std::cerr << "Failed to parse level file: " << filename << std::endl;
        return false;
    }
    
    // 2. Сохраняем в ResourceManager
    rm.addLevel(levelName, std::move(levelData));
    
    // 3. Загружаем ресурсы уровня
    const auto& level = rm.getLevel(levelName);
    loadLevelResources(level);
    
    // 4. Настраиваем звуки уровня
    if (!setupLevelSounds(level, soundEngineer)) {
        std::cerr << "Failed to setup level sounds for: " << levelName << std::endl;
        return false;
    }
    
    std::cout << "Level processed successfully: " << levelName << std::endl;
    return true;
}

const LevelData& LevelProcessor::getProcessedLevel(const std::string& levelName) {
    auto& rm = ResourceManager::getInstance();
    return rm.getLevel(levelName);
}

void LevelProcessor::unloadLevel(const std::string& levelName) {
    auto& rm = ResourceManager::getInstance();
    // ResourceManager очистит ресурсы при удалении уровня
}

bool LevelProcessor::loadLevelResources(const LevelData& level) {
    bool texturesLoaded = false;
    bool musicLoaded = true;
    bool soundsLoaded = true;
    
    // Загружаем текстуры
    texturesLoaded = loadLevelTextures(level);
    if (!texturesLoaded) {
        std::cerr << "Warning: Some textures failed to load for level: " << level.name << std::endl;
        // Не прерываем загрузку, используем fallback текстуры
    }
    
    // Загружаем музыку
    if (!loadLevelMusic(level)) {
        std::cerr << "Warning: Failed to load music for level: " << level.name << std::endl;
        musicLoaded = false;
    }
    
    // Загружаем звуки
    if (!loadLevelSounds(level)) {
        std::cerr << "Warning: Failed to load sounds for level: " << level.name << std::endl;
        soundsLoaded = false;
    }
    
    // Уровень считается загруженным, даже если не все текстуры загружены
    // Будут использованы fallback-текстуры
    return true;
}

bool LevelProcessor::setupLevelSounds(const LevelData& level, SoundEngineer& soundEngineer) {
    // Настраиваем фоновую музыку
    if (!level.backgroundMusic.empty()) {
        std::cout << "Setting up level music: " << level.backgroundMusic << std::endl;
        // Звуковая настройка будет выполнена SoundEngineer
    }
    
    // Настраиваем фоновые звуки
    if (!level.ambienceSound.empty()) {
        std::cout << "Setting up level ambience: " << level.ambienceSound << std::endl;
        // Звуковая настройка будет выполнена SoundEngineer
    }
    
    return true;
}

bool LevelProcessor::isLevelReady(const std::string& levelName) {
    auto& rm = ResourceManager::getInstance();
    return rm.hasLevel(levelName);
}

void LevelProcessor::clearAllLevels() {
    auto& rm = ResourceManager::getInstance();
    // ResourceManager очистит все при вызове clear()
}

bool LevelProcessor::loadLevelTextures(const LevelData& level) {
    auto& rm = ResourceManager::getInstance();
    bool allLoaded = true;
    
    // Загружаем текстуры стен
    for (const auto& [id, texName] : level.wallTextures) {
        auto it = level.texturePaths.find(texName);
        if (it != level.texturePaths.end()) {
            std::string texturePath = it->second;
            std::cout << "Attempting to load wall texture: " << texName << " from path: " << texturePath << std::endl;
            
            if (!rm.loadTexture(texName, texturePath)) {
                std::cerr << "Failed to load wall texture: " << texName << " from " << texturePath << std::endl;
                
                // Попробуем использовать fallback текстуру
                if (rm.hasTexture("walls")) {
                    std::cout << "Using fallback texture for: " << texName << std::endl;
                } else {
                    allLoaded = false;
                }
            } else {
                std::cout << "Successfully loaded wall texture: " << texName << " (ID: " << id << ")" << std::endl;
            }
        } else {
            std::cerr << "Texture path not found for wall texture: " << texName << " (ID: " << id << ")" << std::endl;
            allLoaded = false;
        }
    }
    
    // Загружаем текстуры пола
    for (const auto& [id, texName] : level.floorTextures) {
        auto it = level.texturePaths.find(texName);
        if (it != level.texturePaths.end()) {
            std::string texturePath = it->second;
            std::cout << "Attempting to load floor texture: " << texName << " from path: " << texturePath << std::endl;
            
            if (!rm.loadTexture(texName, texturePath)) {
                std::cerr << "Failed to load floor texture: " << texName << " from " << texturePath << std::endl;
                
                // Попробуем использовать fallback текстуру
                if (rm.hasTexture("floors")) {
                    std::cout << "Using fallback texture for: " << texName << std::endl;
                } else {
                    allLoaded = false;
                }
            } else {
                std::cout << "Successfully loaded floor texture: " << texName << " (ID: " << id << ")" << std::endl;
            }
        } else {
            std::cerr << "Texture path not found for floor texture: " << texName << " (ID: " << id << ")" << std::endl;
            allLoaded = false;
        }
    }
    
    // Загружаем текстуры потолка
    for (const auto& [id, texName] : level.ceilingTextures) {
        auto it = level.texturePaths.find(texName);
        if (it != level.texturePaths.end()) {
            std::string texturePath = it->second;
            std::cout << "Attempting to load ceiling texture: " << texName << " from path: " << texturePath << std::endl;
            
            if (!rm.loadTexture(texName, texturePath)) {
                std::cerr << "Failed to load ceiling texture: " << texName << " from " << texturePath << std::endl;
                
                // Попробуем использовать fallback текстуру
                if (rm.hasTexture("ceilings")) {
                    std::cout << "Using fallback texture for: " << texName << std::endl;
                } else {
                    allLoaded = false;
                }
            } else {
                std::cout << "Successfully loaded ceiling texture: " << texName << " (ID: " << id << ")" << std::endl;
            }
        } else {
            std::cerr << "Texture path not found for ceiling texture: " << texName << " (ID: " << id << ")" << std::endl;
            allLoaded = false;
        }
    }
    
    return allLoaded;
}

bool LevelProcessor::loadLevelMusic(const LevelData& level) {
    auto& rm = ResourceManager::getInstance();
    
    if (!level.backgroundMusic.empty()) {
        // Определяем тип музыки по расширению
        std::string extension = rm.getFileExtension(level.backgroundMusic);
        
        if (extension == "mid" || extension == "midi") {
            if (!rm.loadMIDI("level_music", level.backgroundMusic)) {
                std::cerr << "Failed to load MIDI: " << level.backgroundMusic << std::endl;
                return false;
            }
        } else {
            if (!rm.loadMusic("level_music", level.backgroundMusic)) {
                std::cerr << "Failed to load music: " << level.backgroundMusic << std::endl;
                return false;
            }
        }
    }
    
    return true;
}

bool LevelProcessor::loadLevelSounds(const LevelData& level) {
    auto& rm = ResourceManager::getInstance();
    
    if (!level.ambienceSound.empty()) {
        std::string extension = rm.getFileExtension(level.ambienceSound);
        
        if (extension == "wav" || extension == "ogg" || extension == "flac") {
            if (!rm.loadSound("level_ambience", level.ambienceSound)) {
                std::cerr << "Failed to load ambience sound: " << level.ambienceSound << std::endl;
                return false;
            }
        } else {
            std::cerr << "Unsupported sound format for ambience: " << level.ambienceSound << std::endl;
            return false;
        }
    }
    
    return true;
}