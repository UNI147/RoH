#include "Game.h"
#include <iostream>
#include "resource_manager/MusicNames.h"
#include "resource_manager/SoundNames.h"

Game::Game(sf::RenderWindow& window) 
    : window_(window) {
    
    loadLevel();
    initializeAudio();
    loadSounds();
    
    renderer_ = std::make_unique<Renderer>(window_);
    
    // Инициализация игрока из данных уровня
    player_.position = currentLevel_.playerStartPosition;
    player_.direction = currentLevel_.playerStartDirection;
    player_.plane = sf::Vector2f(0.0f, 0.66f);
}

void Game::loadLevel() {
    std::cout << "Loading level resources..." << std::endl;
    
    // Загружаем уровень через MapLoader (включая все ресурсы)
    currentLevel_ = mapLoader_.loadLevel("resources/levels/test_level.roh");
    
    // Если файл уровня не найден, используем тестовый уровень
    if (currentLevel_.grid.empty()) {
        std::cout << "Using test level..." << std::endl;
        currentLevel_ = mapLoader_.createTestLevel();
    }
    
    std::cout << "Level loaded successfully!" << std::endl;
    std::cout << "Grid size: " << currentLevel_.grid.size() << "x" 
              << (currentLevel_.grid.empty() ? 0 : currentLevel_.grid[0].size()) << std::endl;
    std::cout << "Textures loaded: " << currentLevel_.textures.size() << std::endl;
    std::cout << "Background music: " << currentLevel_.backgroundMusic << std::endl;
}

void Game::initializeAudio() {
    // Загружаем MIDI файл через SoundEngineer
    if (!currentLevel_.backgroundMusic.empty()) {
        if (soundEngineer_.loadMIDI(Music::ADRIANS_ASLEEP, currentLevel_.backgroundMusic)) {
            soundEngineer_.playMIDI(Music::ADRIANS_ASLEEP, true);
            soundEngineer_.setMusicVolume(50.0f);
            std::cout << "Playing level music as MIDI: " << Music::ADRIANS_ASLEEP << std::endl;
        } else {
            std::cerr << "Failed to load MIDI music: " << currentLevel_.backgroundMusic << std::endl;
        }
    } else {
        std::cerr << "No background music specified for level!" << std::endl;
    }
}

void Game::loadSounds() {
    auto& rm = ResourceManager::getInstance();
    
    // Загружаем звук шагов
    std::vector<std::string> stepPaths = {
        "resources/sounds/effects/step.wav",
        "../resources/sounds/effects/step.wav",
        "../../resources/sounds/effects/step.wav"
    };
    
    bool stepLoaded = false;
    for (const auto& path : stepPaths) {
        if (rm.loadSound(Sounds::STEP, path)) {
            stepLoaded = true;
            std::cout << "Step sound loaded from: " << path << std::endl;
            break;
        }
    }
    
    if (!stepLoaded) {
        std::cerr << "Failed to load step sound effect!" << std::endl;
    }
}

void Game::update() {
    float deltaTime = clock_.restart().asSeconds();
    
    handleEvents();
    
    // Сохраняем старое положение для определения движения
    sf::Vector2f oldPosition = player_.position;
    
    // Обрабатываем ввод
    inputHandler_.handleInput(player_, deltaTime, currentLevel_.grid);
    
    // Определяем, двигается ли игрок
    bool isMoving = (player_.position != oldPosition);
    bool isRunning = inputHandler_.isRunning();
    
    // Обновляем звуки шагов
    soundEngineer_.updateFootsteps(isMoving, isRunning, deltaTime);
}

void Game::render() {
    window_.clear();
    renderer_->renderFrame(player_, currentLevel_.grid, rayCaster_);
    window_.display();
}

void Game::handleEvents() {
    sf::Event event;
    while (window_.pollEvent(event)) {
        if (event.type == sf::Event::Closed)
            window_.close();
        
        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape)
            window_.close();
    }
}