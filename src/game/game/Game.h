#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include "engine/raycasting/RayCaster.h"
#include "engine/renderer/Renderer.h"
#include "engine/geographer/LevelProcessor.h"
#include "engine/geographer/MapLoader.h"
#include "sound_engineer/SoundEngineer.h"
#include "input_handler/InputHandler.h"
#include "resource_manager/ResourceManager.h"

class MapLoader;

class Game {
public:
    Game(sf::RenderWindow& window);
    void update();
    void render();

private:
    sf::RenderWindow& window_;
    sf::Clock clock_;
    
    // Системы игры
    RayCaster rayCaster_;
    std::unique_ptr<Renderer> renderer_;
    InputHandler inputHandler_;
    SoundEngineer soundEngineer_;
    
    // Состояние игры
    PlayerState player_;
    LevelData currentLevel_;
    
    // Состояние инициализации
    bool gameReady_ = false;
    sf::Text loadingText_;
    
    // Элементы интерфейса загрузки
    sf::RectangleShape progressBarBackground_;
    sf::RectangleShape progressBar_;
    sf::Text loadingTitle_;
    
    void handleEvents();
    void loadLevel();
    void loadSounds();
    void initializeResources();
    void setupLoadingScreen();
    void setupRendererTextures();
    void playBackgroundMusic();
    void startAmbientSounds();
    void renderLoadingScreen();
    void updateLoadingScreen(float progress);
    
    void createFallbackLevel();
    
    bool checkFurnitureCollision(const sf::Vector2f& position) const;
};