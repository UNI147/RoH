#include "Game.h"
#include <iostream>
#include "resource_manager/MusicNames.h"
#include "resource_manager/SoundNames.h"
#include "resource_manager/FontNames.h"

Game::Game(sf::RenderWindow& window) 
    : window_(window) {
    
    // Инициализация ресурсов
    initializeResources();
    
    // Настройка элементов интерфейса загрузки
    setupLoadingScreen();
    
    loadLevel();
    loadSounds();
    
    renderer_ = std::make_unique<Renderer>(window_);
    
    // Инициализация игрока из данных уровня
    player_.position = currentLevel_.playerStartPosition;
    player_.direction = currentLevel_.playerStartDirection;
    player_.plane = sf::Vector2f(0.0f, 0.66f);
}

void Game::setupLoadingScreen() {
    auto& rm = ResourceManager::getInstance();
    
    // Используем основной шрифт или fallback
    if (rm.hasFont(Fonts::SPECIAL_ELITE)) {
        loadingTitle_.setFont(rm.getFont(Fonts::SPECIAL_ELITE));
        loadingText_.setFont(rm.getFont(Fonts::SPECIAL_ELITE));
    } else if (rm.hasFont(Fonts::SYSTEM_FALLBACK)) {
        loadingTitle_.setFont(rm.getFont(Fonts::SYSTEM_FALLBACK));
        loadingText_.setFont(rm.getFont(Fonts::SYSTEM_FALLBACK));
    } else {
        // Если шрифты не загружены, это критическая ошибка
        std::cerr << "No fonts available for loading screen!" << std::endl;
    }
    
    // Настройка текстовых элементов
    loadingTitle_.setString("RoH Demo");
    loadingTitle_.setCharacterSize(36);
    loadingTitle_.setFillColor(sf::Color::Red);
    loadingTitle_.setStyle(sf::Text::Bold);
    
    loadingText_.setCharacterSize(18);
    loadingText_.setFillColor(sf::Color(128, 128, 128));
    
    // Полоса прогресса
    progressBarBackground_.setSize(sf::Vector2f(400.0f, 25.0f));
    progressBarBackground_.setFillColor(sf::Color(50, 50, 50));
    progressBarBackground_.setOutlineThickness(2.0f);
    progressBarBackground_.setOutlineColor(sf::Color(128, 128, 128));
    
    progressBar_.setSize(sf::Vector2f(0.0f, 21.0f));
    progressBar_.setFillColor(sf::Color::Red);
}

void Game::initializeResources() {
    auto& rm = ResourceManager::getInstance();
    
    // Загрузка шрифтов
    std::vector<std::pair<std::string, std::string>> fontPaths = {
        {Fonts::SPECIAL_ELITE, "resources/SpecialElite.ttf"},
        {Fonts::GOTHIC_RUS, "resources/GothicRus.ttf"}
    };
    
    for (const auto& [name, path] : fontPaths) {
        if (!rm.loadFont(name, path)) {
            std::cerr << "Failed to load font: " << name << " from " << path << std::endl;
        }
    }
    
    // Загрузка системного fallback шрифта
    if (!rm.hasFont(Fonts::SPECIAL_ELITE) && !rm.hasFont(Fonts::GOTHIC_RUS)) {
        if (rm.loadFont(Fonts::SYSTEM_FALLBACK, "C:/Windows/Fonts/arial.ttf")) {
            std::cout << "Using system fallback font" << std::endl;
        } else {
            std::cerr << "Failed to load system fallback font!" << std::endl;
        }
    }
    
    // Загрузка SoundFont
    rm.loadSoundFont("default", "resources/OPL3SB.sf2");
    
    // Загрузка MIDI файлов
    std::vector<std::string> midiPaths = {
        "resources/sounds/music/Adrian'sAsleep.mid",
        "resources/sounds/music/adrians_asleep.mid",
        "resources/sounds/music/AdriansAsleep.mid"
    };
    
    bool midiLoaded = false;
    for (const auto& path : midiPaths) {
        if (rm.loadMIDI(Music::ADRIANS_ASLEEP, path)) {
            midiLoaded = true;
            std::cout << "MIDI loaded: " << path << std::endl;
            break;
        }
    }
    
    if (!midiLoaded) {
        std::cerr << "Failed to load MIDI file with any path variant" << std::endl;
    }
    
    // Загрузка звуков
    rm.loadSound(Sounds::STEP, "resources/sounds/effects/step.wav");
    
    // Инициализация звукового движка
    soundEngineer_.initializeFluidSynth("default");
}

void Game::updateLoadingScreen(float progress) {
    // Обновляем текст загрузки
    int progressPercent = static_cast<int>(progress * 100);
    loadingText_.setString("Initializing audio system... " + std::to_string(progressPercent) + "%");
    
    // Обновляем полосу прогресса с градиентом
    float progressBarWidth = 396.0f * progress;
    progressBar_.setSize(sf::Vector2f(progressBarWidth, 21.0f));
    
    // Градиент от черно-красного к бело-желтому
    if (progress < 0.5f) {
        // От черного к красному (0.0 - 0.5)
        float t = progress * 2.0f;
        sf::Uint8 red = static_cast<sf::Uint8>(255 * t);
        progressBar_.setFillColor(sf::Color(red, 0, 0));
    } else {
        // От красного к желто-белому (0.5 - 1.0)
        float t = (progress - 0.5f) * 2.0f;
        sf::Uint8 red = 255;
        sf::Uint8 green = static_cast<sf::Uint8>(255 * t);
        sf::Uint8 blue = static_cast<sf::Uint8>(255 * t);
        progressBar_.setFillColor(sf::Color(red, green, blue));
    }
}

void Game::renderLoadingScreen() {
    // Получаем размеры окна рендерера (игрового разрешения)
    unsigned int renderWidth = 320;
    unsigned int renderHeight = 200;
    
    // Создаем временную текстуру для рендеринга в игровом разрешении
    sf::RenderTexture renderTexture;
    if (!renderTexture.create(renderWidth, renderHeight)) {
        // Если не удалось создать текстуру, используем fallback
        window_.clear(sf::Color::Black);
        
        // Центрируем элементы относительно окна
        sf::Vector2u windowSize = window_.getSize();
        
        loadingTitle_.setPosition(
            (windowSize.x - loadingTitle_.getLocalBounds().width) / 2.0f,
            windowSize.y * 0.3f
        );
        
        progressBarBackground_.setPosition(
            (windowSize.x - 400.0f) / 2.0f,
            windowSize.y * 0.5f
        );
        
        progressBar_.setPosition(
            progressBarBackground_.getPosition().x + 2.0f,
            progressBarBackground_.getPosition().y + 2.0f
        );
        
        loadingText_.setPosition(
            (windowSize.x - loadingText_.getLocalBounds().width) / 2.0f,
            progressBarBackground_.getPosition().y + 35.0f
        );
        
        window_.draw(loadingTitle_);
        window_.draw(progressBarBackground_);
        window_.draw(progressBar_);
        window_.draw(loadingText_);
        return;
    }
    
    // Рендерим в текстуру с игровым разрешением
    renderTexture.clear(sf::Color::Black);
    
    // Центрируем элементы относительно игрового разрешения
    loadingTitle_.setPosition(
        (renderWidth - loadingTitle_.getLocalBounds().width) / 2.0f,
        renderHeight * 0.3f
    );
    
    progressBarBackground_.setPosition(
        (renderWidth - 400.0f) / 2.0f,
        renderHeight * 0.5f
    );
    
    progressBar_.setPosition(
        progressBarBackground_.getPosition().x + 2.0f,
        progressBarBackground_.getPosition().y + 2.0f
    );
    
    loadingText_.setPosition(
        (renderWidth - loadingText_.getLocalBounds().width) / 2.0f,
        progressBarBackground_.getPosition().y + 35.0f
    );
    
    // Рисуем элементы на текстуре
    renderTexture.draw(loadingTitle_);
    renderTexture.draw(progressBarBackground_);
    renderTexture.draw(progressBar_);
    renderTexture.draw(loadingText_);
    
    renderTexture.display();
    
    // Отображаем текстуру на экране с масштабированием
    sf::Sprite renderSprite(renderTexture.getTexture());
    
    // Масштабируем до размера окна (как в основном рендерере)
    sf::Vector2u windowSize = window_.getSize();
    float scaleX = static_cast<float>(windowSize.x) / static_cast<float>(renderWidth);
    float scaleY = static_cast<float>(windowSize.y) / static_cast<float>(renderHeight);
    float scale = std::min(scaleX, scaleY);
    
    renderSprite.setScale(scale, scale);
    
    // Центрируем спрайт
    float offsetX = (static_cast<float>(windowSize.x) - static_cast<float>(renderWidth) * scale) / 2.0f;
    float offsetY = (static_cast<float>(windowSize.y) - static_cast<float>(renderHeight) * scale) / 2.0f;
    renderSprite.setPosition(offsetX, offsetY);
    
    window_.clear(sf::Color::Black);
    window_.draw(renderSprite);
}

void Game::update() {
    float deltaTime = clock_.restart().asSeconds();
    
    handleEvents();
    
    // Обновляем инициализацию аудиосистемы
    if (!gameReady_) {
        soundEngineer_.updateInitialization(deltaTime);
        float progress = soundEngineer_.getInitializationProgress();
        
        // Обновляем интерфейс загрузки
        updateLoadingScreen(progress);
        
        // Проверяем, готовы ли начать игру
        if (soundEngineer_.isAudioReady()) {
            gameReady_ = true;
            playBackgroundMusic();
            std::cout << "Ready to start" << std::endl;
        }
        return;
    }
    
    // Основной игровой цикл (только когда игра готова)
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
    
    if (!gameReady_) {
        // Показываем экран загрузки
        renderLoadingScreen();
    } else {
        // Основной рендеринг игры
        renderer_->renderFrame(player_, currentLevel_.grid, rayCaster_);
    }
    
    window_.display();
}

void Game::playBackgroundMusic() {
    if (!currentLevel_.backgroundMusic.empty() && soundEngineer_.isAudioReady()) {
        std::vector<std::string> possibleNames = {
            Music::ADRIANS_ASLEEP,
            "Adrian'sAsleep",
            "adrians_asleep"
        };
        
        for (const auto& name : possibleNames) {
            if (ResourceManager::getInstance().hasMIDI(name)) {
                soundEngineer_.playMIDI(name, true);
                soundEngineer_.setMusicVolume(100.0f);
                std::cout << "Playing background music: " << name << std::endl;
                return;
            }
        }
        
        std::cout << "Trying to play music from level path: " << currentLevel_.backgroundMusic << std::endl;
        soundEngineer_.playMIDI(currentLevel_.backgroundMusic, true);
    }
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

void Game::loadLevel() {
    std::cout << "Loading level resources..." << std::endl;
    
    if (MapLoader::loadLevel("test_level", "resources/levels/test_level.roh")) {
        currentLevel_ = MapLoader::getLevel("test_level");
        std::cout << "Level loaded successfully!" << std::endl;
        
        // Загружаем текстуры из данных уровня
        auto& rm = ResourceManager::getInstance();
        for (const auto& [name, path] : currentLevel_.textures) {
            if (!rm.loadTexture(name, path)) {
                std::cerr << "Failed to load texture: " << name << " from " << path << std::endl;
            } else {
                std::cout << "Successfully loaded texture: " << name << std::endl;
            }
        }
    } else {
        std::cout << "Using test level..." << std::endl;
        auto& rm = ResourceManager::getInstance();
        currentLevel_ = rm.createTestLevel();
    }
}

void Game::loadSounds() {
    auto& rm = ResourceManager::getInstance();
    
    // Загружаем звук шагов
    if (!rm.hasSound(Sounds::STEP)) {
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
}