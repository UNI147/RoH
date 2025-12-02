#include "Game.h"
#include <iostream>
#include "resource_manager/FontNames.h"

Game::Game(sf::RenderWindow& window) 
    : window_(window) {
    
    // Инициализация ресурсов
    initializeResources();
    
    // Инициализация звуковой системы
    if (!soundEngineer_.initializeFluidSynth("default")) {
        std::cerr << "Warning: FluidSynth initialization failed, continuing without MIDI" << std::endl;
    }
    
    // Настройка элементов интерфейса загрузки
    setupLoadingScreen();
    
    // Загружаем уровень ДО инициализации рендерера и игрока
    loadLevel();
    
    // Инициализируем рендерер после загрузки уровня
    renderer_ = std::make_unique<Renderer>(window_);
    
    // Настраиваем текстуры рендерера из данных уровня
    setupRendererTextures();
    
    // Инициализация игрока из данных уровня
    player_.position = currentLevel_.playerStartPosition;
    player_.direction = currentLevel_.playerStartDirection;
    player_.plane = sf::Vector2f(0.0f, 0.66f);
    
    // Загружаем звуки после загрузки уровня
    loadSounds();
}

void Game::setupRendererTextures() {
    if (!currentLevel_.floorTexture.empty()) {
        renderer_->setFloorTexture(currentLevel_.floorTexture);
    }
    if (!currentLevel_.ceilingTexture.empty()) {
        renderer_->setCeilingTexture(currentLevel_.ceilingTexture);
    }
    
    // Устанавливаем текстуры стен на основе данных уровня
    for (const auto& [texName, texPath] : currentLevel_.textures) {
        // Пропускаем текстуры пола и потолка, они уже установлены
        if (texName == currentLevel_.floorTexture || texName == currentLevel_.ceilingTexture) {
            continue;
        }
        // Остальные текстуры считаем текстурами стен
        renderer_->addWallTexture(texName);
    }
}

void Game::setupLoadingScreen() {
    auto& rm = ResourceManager::getInstance();
    
    // Используем GothicRus для заголовка и SpecialElite для текста
    if (rm.hasFont(Fonts::GOTHIC_RUS)) {
        loadingTitle_.setFont(rm.getFont(Fonts::GOTHIC_RUS));
        std::cout << "Using GothicRus font for loading title" << std::endl;
    } else if (rm.hasFont(Fonts::SPECIAL_ELITE)) {
        loadingTitle_.setFont(rm.getFont(Fonts::SPECIAL_ELITE));
        std::cout << "Using SpecialElite font for loading title (fallback)" << std::endl;
    } else if (rm.hasFont(Fonts::SYSTEM_FALLBACK)) {
        loadingTitle_.setFont(rm.getFont(Fonts::SYSTEM_FALLBACK));
        std::cout << "Using system fallback font for loading title" << std::endl;
    } else {
        std::cerr << "No fonts available for loading screen title!" << std::endl;
    }
    
    // Для основного текста используем SpecialElite
    if (rm.hasFont(Fonts::SPECIAL_ELITE)) {
        loadingText_.setFont(rm.getFont(Fonts::SPECIAL_ELITE));
    } else if (rm.hasFont(Fonts::GOTHIC_RUS)) {
        loadingText_.setFont(rm.getFont(Fonts::GOTHIC_RUS));
    } else if (rm.hasFont(Fonts::SYSTEM_FALLBACK)) {
        loadingText_.setFont(rm.getFont(Fonts::SYSTEM_FALLBACK));
    } else {
        std::cerr << "No fonts available for loading text!" << std::endl;
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
    rm.setBasePath("resources/");
    
    // Загружаем шрифты
    if (!rm.loadFont(Fonts::GOTHIC_RUS, "GothicRus.ttf")) {
        std::cerr << "Failed to load Gothic font!" << std::endl;
    }
    
    if (!rm.loadFont(Fonts::SPECIAL_ELITE, "SpecialElite.ttf")) {
        std::cerr << "CRITICAL: Failed to load main font!" << std::endl;
    }
    
    // Звуки
    rm.loadSound("step", "sounds/effects/step.wav");
    rm.loadSound("drops", "sounds/effects/drops.wav");
    
    // SoundFont
    rm.loadSoundFont("default", "OPL3SB.sf2");
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
            startAmbientSounds(); // Запускаем фоновые звуки
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

void Game::startAmbientSounds() {
    // Запускаем фоновый звук
    if (!currentLevel_.ambienceSound.empty()) {
        soundEngineer_.startAmbience("level_ambience", 100.0f);
        std::cout << "Started level ambience: " << currentLevel_.ambienceSound << std::endl;
    } else {
        // Fallback на системный эмбиент, если в уровне не указан
        soundEngineer_.startAmbience("ambienceloop", 25.0f);
        std::cout << "Using default ambience (level ambience not specified)" << std::endl;
    }
    
    // Добавляем случайные звуки с интервалом 15-45 секунд
    soundEngineer_.addRandomSound("drops", 15.0f, 45.0f, 100.0f);
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
    // Используем музыку из данных уровня
    if (!currentLevel_.backgroundMusic.empty() && soundEngineer_.isAudioReady()) {
        std::cout << "Attempting to play level music: " << currentLevel_.backgroundMusic << std::endl;
        
        // Пробуем воспроизвести как MIDI
        soundEngineer_.playMIDI("level_music", true);
        soundEngineer_.setMusicVolume(100.0f);
        
        std::cout << "Playing background music from level: " << currentLevel_.backgroundMusic << std::endl;
    } else {
        std::cout << "Cannot play background music - audio not ready or no music specified" << std::endl;
        if (!soundEngineer_.isAudioReady()) {
            std::cout << "Audio system not ready" << std::endl;
        }
        if (currentLevel_.backgroundMusic.empty()) {
            std::cout << "No background music specified in level" << std::endl;
        }
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
    
    // Используем LevelProcessor для обработки уровня
    if (LevelProcessor::processLevel("current_level", "levels/test_level.roh", soundEngineer_)) {
        currentLevel_ = LevelProcessor::getProcessedLevel("current_level");
        std::cout << "Level '" << currentLevel_.name << "' loaded and processed successfully!" << std::endl;
    } else {
        std::cout << "Using test level..." << std::endl;
        auto& rm = ResourceManager::getInstance();
        
        // Исправленная строка: разыменовываем указатель
        std::unique_ptr<LevelData> testLevel = rm.createTestLevel();
        if (testLevel) {
            currentLevel_ = *testLevel;  // Копируем данные
        } else {
            // Fallback если создание тестового уровня не удалось
            currentLevel_.name = "Fallback Test Level";
            currentLevel_.grid = {
                {1, 1, 1, 1},
                {1, 0, 0, 1},
                {1, 0, 0, 1},
                {1, 1, 1, 1}
            };
            currentLevel_.playerStartPosition = sf::Vector2f(1.5f, 1.5f);
            currentLevel_.playerStartDirection = sf::Vector2f(-1.0f, 0.0f);
        }
        
        // Загружаем ресурсы тестового уровня
        LevelProcessor::loadLevelResources(currentLevel_);
    }
}

void Game::loadSounds() {
    auto& rm = ResourceManager::getInstance();
    
    // Загружаем системные звуки - используем прямые имена
    if (!rm.hasSound("step")) {
        std::cerr << "Critical error: Step sound not loaded!" << std::endl;
    }
    
    if (!rm.hasSound("ambienceloop")) {
        std::cerr << "Warning: Ambience sound not loaded" << std::endl;
    }
    
    if (!rm.hasSound("drops")) {
        std::cerr << "Warning: Drops sound not loaded" << std::endl;
    }
}