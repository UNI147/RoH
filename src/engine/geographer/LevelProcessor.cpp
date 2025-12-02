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
    if (!loadLevelResources(level)) {
        std::cerr << "Failed to load level resources for: " << levelName << std::endl;
        return false;
    }
    
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
    // Дополнительная очистка звуков может быть добавлена здесь
}

bool LevelProcessor::loadLevelResources(const LevelData& level) {
    bool success = true;
    
    // Загружаем текстуры
    if (!loadLevelTextures(level)) {
        std::cerr << "Failed to load textures for level: " << level.name << std::endl;
        success = false;
    }
    
    // Загружаем музыку
    if (!loadLevelMusic(level)) {
        std::cerr << "Failed to load music for level: " << level.name << std::endl;
        success = false;
    }
    
    // Загружаем звуки
    if (!loadLevelSounds(level)) {
        std::cerr << "Failed to load sounds for level: " << level.name << std::endl;
        success = false;
    }
    
    return success;
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
    
    for (const auto& [texName, texPath] : level.textures) {
        if (!rm.loadTexture(texName, texPath)) {
            std::cerr << "Failed to load texture: " << texName << " from " << texPath << std::endl;
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