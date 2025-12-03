#include "resource_manager/ResourceManager.h"
#include <iostream>

class TexturePreloader {
public:
    static void preloadCommonTextures() {
        auto& rm = ResourceManager::getInstance();
        
        std::cout << "Preloading common textures..." << std::endl;
        
        // Список общих текстур для всех уровней
        std::vector<std::pair<std::string, std::string>> commonTextures = {
            {"wall_1", "textures/surfaces/bricksbloody.png"},
            {"wall_2", "textures/surfaces/brickwall.png"},
            {"wall_3", "textures/surfaces/boards.png"},
            {"floor_1", "textures/surfaces/parquet.png"},
            {"floor_2", "textures/surfaces/carpet.png"},
            {"ceiling_1", "textures/surfaces/boards.png"},
            {"ceiling_2", "textures/surfaces/patterned.png"},
            {"walls", "textures/surfaces/bricksbloody.png"},
            {"floors", "textures/surfaces/parquet.png"},
            {"ceilings", "textures/surfaces/boards.png"}
        };
        
        bool success = rm.loadResourceBatch(commonTextures);
        if (success) {
            std::cout << "Successfully preloaded common textures" << std::endl;
        } else {
            std::cout << "Failed to preload some common textures" << std::endl;
        }
    }
    
    static void preloadLevelTextures(const std::string& levelName) {
        std::cout << "Preloading textures for level: " << levelName << std::endl;
        
        // Можно добавить специфичную для уровня логику здесь
        preloadCommonTextures();
    }
};