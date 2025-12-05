#include "Game.h"
#include <iostream>
#include "resource_manager/FontNames.h"
#include "engine/geographer/FurnitureLoader.h"

Game::Game(sf::RenderWindow& window) 
    : window_(window) {
    
    // Инициализация ресурсов (только шрифты и звуки)
    initializeResources();
    
    // Инициализация звуковой системы
    if (!soundEngineer_.initializeFluidSynth("default")) {
        std::cerr << "Warning: FluidSynth initialization failed, continuing without MIDI" << std::endl;
    }
    
    // Настройка элементов интерфейса загрузки
    setupLoadingScreen();
    
    // Загружаем уровень ДО инициализации рендерера
    loadLevel();
    
    // Инициализируем рендерер ПОСЛЕ загрузки уровня
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
    auto& rm = ResourceManager::getInstance();
    
    std::cout << "=== Setting up renderer textures ===" << std::endl;
    
    // Очищаем старые текстуры
    renderer_->clearTextures();
    
    // Загружаем текстуры стен
    for (const auto& [texId, texName] : currentLevel_.wallTextures) {
        std::cout << "Setting up wall texture ID " << texId << ": " << texName << std::endl;
        
        if (rm.hasTexture(texName)) {
            renderer_->addWallTexture(texId, texName);
            std::cout << "Texture loaded" << std::endl;
        } else {
            std::cout << "Texture not found in ResourceManager" << std::endl;
            
            // Попробуем загрузить напрямую
            auto it = currentLevel_.texturePaths.find(texName);
            if (it != currentLevel_.texturePaths.end()) {
                if (rm.loadTexture(texName, it->second)) {
                    renderer_->addWallTexture(texId, texName);
                    std::cout << "Texture loaded directly" << std::endl;
                }
            }
        }
    }
    
    // Загружаем текстуры пола
    for (const auto& [texId, texName] : currentLevel_.floorTextures) {
        std::cout << "Setting up floor texture ID " << texId << ": " << texName << std::endl;
        
        if (rm.hasTexture(texName)) {
            renderer_->addFloorTexture(texId, texName);
            std::cout << "Texture loaded" << std::endl;
        } else {
            std::cout << "Texture not found in ResourceManager" << std::endl;
            
            auto it = currentLevel_.texturePaths.find(texName);
            if (it != currentLevel_.texturePaths.end()) {
                if (rm.loadTexture(texName, it->second)) {
                    renderer_->addFloorTexture(texId, texName);
                    std::cout << "Texture loaded directly" << std::endl;
                }
            }
        }
    }
    
    // Загружаем текстуры потолка
    for (const auto& [texId, texName] : currentLevel_.ceilingTextures) {
        std::cout << "Setting up ceiling texture ID " << texId << ": " << texName << std::endl;
        
        if (rm.hasTexture(texName)) {
            renderer_->addCeilingTexture(texId, texName);
            std::cout << "Texture loaded" << std::endl;
        } else {
            std::cout << "Texture not found in ResourceManager" << std::endl;
            
            auto it = currentLevel_.texturePaths.find(texName);
            if (it != currentLevel_.texturePaths.end()) {
                if (rm.loadTexture(texName, it->second)) {
                    renderer_->addCeilingTexture(texId, texName);
                    std::cout << "Texture loaded directly" << std::endl;
                }
            }
        }
    }
    
    // Проверяем, есть ли вообще текстуры (используя публичные методы)
    if (!renderer_->hasTextures()) {
        renderer_->setUseTextures(false);
        std::cout << "Warning: No textures loaded, falling back to color rendering" << std::endl;
    } else {
        renderer_->setUseTextures(true);
        std::cout << "Textured rendering enabled with " 
                << renderer_->getWallTextureCount() << " wall textures, "
                << renderer_->getFloorTextureCount() << " floor textures, "
                << renderer_->getCeilingTextureCount() << " ceiling textures" << std::endl;
    }

    std::cout << "=== Loading furniture textures ===" << std::endl;
    std::cout << "Found " << currentLevel_.furnitureTypes.size() << " furniture types" << std::endl;
    std::cout << "Found " << currentLevel_.furnitureObjects.size() << " furniture objects" << std::endl;
    
    // Загружаем текстуры для каждого типа мебели
    for (const auto& [furnitureName, furnitureData] : currentLevel_.furnitureTypes) {
        std::cout << "Processing furniture: " << furnitureName << std::endl;
        std::cout << "  Texture name: " << furnitureData.textureName << std::endl;
        
        // Загружаем текстуру мебели в ResourceManager
        std::string textureKey = "furniture_" + furnitureName;
        
        if (!rm.hasTexture(textureKey)) {
            // Пытаемся найти файл текстуры
            std::string foundPath = rm.findResourceFile(furnitureData.textureName);
            if (!foundPath.empty()) {
                if (rm.loadTexture(textureKey, foundPath)) {
                    std::cout << "  Loaded furniture texture: " << textureKey << std::endl;
                    renderer_->addFurnitureTexture(furnitureName, textureKey);
                } else {
                    std::cout << "  FAILED to load furniture texture: " << furnitureData.textureName << std::endl;
                }
            } else {
                std::cout << "  Texture file not found: " << furnitureData.textureName << std::endl;
            }
        } else {
            std::cout << "  Furniture texture already loaded: " << textureKey << std::endl;
            renderer_->addFurnitureTexture(furnitureName, textureKey);
        }
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
    
    // Загружаем шрифты и звуки
    if (!rm.loadFont(Fonts::GOTHIC_RUS, "GothicRus.ttf")) {
        std::cerr << "Failed to load Gothic font!" << std::endl;
    }
    
    if (!rm.loadFont(Fonts::SPECIAL_ELITE, "SpecialElite.ttf")) {
        std::cerr << "CRITICAL: Failed to load main font!" << std::endl;
    }
    
    // Звуки
    rm.loadSound("step", "sounds/effects/step.wav");
    rm.loadSound("drops", "sounds/effects/drops.wav");
    rm.loadSound("ambienceloop", "sounds/effects/ambienceloop.wav");
    
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
            startAmbientSounds();
            playBackgroundMusic();
            std::cout << "Ready to start" << std::endl;
        }
        return;
    }
    
    // Основной игровой цикл (только когда игра готова)
    sf::Vector2f oldPosition = player_.position;
    
    // Обрабатываем ввод
    inputHandler_.handleInput(player_, deltaTime, 
                              currentLevel_.wallGrid,
                              currentLevel_.furnitureObjects,
                              currentLevel_.furnitureTypes);
    
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
        renderLoadingScreen();
    } else {
        // Собираем источники света из мебели
        if (renderer_) {
            renderer_->collectLightSourcesForScene(
                currentLevel_.furnitureObjects,
                currentLevel_.furnitureTypes
            );
        }
        
        // Основной рендеринг игры
        renderer_->renderFrame(player_, 
                               currentLevel_.wallGrid,
                               currentLevel_.floorGrid,
                               currentLevel_.ceilingGrid,
                               rayCaster_);
        
        // Теперь рендерим мебель
        if (renderer_->hasTextures() && !currentLevel_.furnitureObjects.empty()) {
            renderer_->renderFurniture(player_,
                                       currentLevel_.furnitureObjects,
                                       currentLevel_.furnitureTypes,
                                       rayCaster_);
        }
        
        renderer_->display();
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
    bool levelLoaded = LevelProcessor::processLevel("current_level", "levels/test_level.roh", soundEngineer_);
    
    if (levelLoaded) {
        currentLevel_ = LevelProcessor::getProcessedLevel("current_level");
        std::cout << "Level '" << currentLevel_.name << "' loaded and processed successfully!" << std::endl;
        
        // ОТЛАДКА: Выводим информацию о текстурах
        std::cout << "Level has " << currentLevel_.wallTextures.size() << " wall textures" << std::endl;
        std::cout << "Level has " << currentLevel_.floorTextures.size() << " floor textures" << std::endl;
        std::cout << "Level has " << currentLevel_.ceilingTextures.size() << " ceiling textures" << std::endl;
        std::cout << "Level has " << currentLevel_.texturePaths.size() << " texture paths" << std::endl;
        
        // Выводим пути к текстурам для отладки
        for (const auto& [name, path] : currentLevel_.texturePaths) {
            std::cout << "Texture path: " << name << " -> " << path << std::endl;
        }
        
        // Проверяем, есть ли вообще текстуры
        auto& rm = ResourceManager::getInstance();
        if (currentLevel_.wallTextures.empty() && !rm.hasTexture("walls")) {
            std::cout << "No wall textures available, creating fallback level..." << std::endl;
            createFallbackLevel();
        }
    } else {
        std::cout << "LevelProcessor failed, creating fallback level..." << std::endl;
        createFallbackLevel();
    }
}

// ДОБАВЛЕН МЕТОД В КЛАСС
void Game::createFallbackLevel() {
    auto& rm = ResourceManager::getInstance();
    
    // Создаем простой уровень для отладки
    currentLevel_.name = "Debug Fallback Level";
    
    // Простая сетка 4x4
    currentLevel_.wallGrid = {
        {1, 1, 1, 1},
        {1, 0, 0, 1},
        {1, 0, 0, 1},
        {1, 1, 1, 1}
    };
    
    // Пол и потолок - те же размеры
    currentLevel_.floorGrid = std::vector<std::vector<int>>(4, std::vector<int>(4, 1));
    currentLevel_.ceilingGrid = std::vector<std::vector<int>>(4, std::vector<int>(4, 1));
    
    // Пробуем загрузить текстуры напрямую через ResourceManager
    std::vector<std::pair<std::string, std::string>> debugTextures = {
        {"wall_1", "textures/surfaces/bricksbloody.png"},
        {"floor_1", "textures/surfaces/parquet.png"},
        {"ceiling_1", "textures/surfaces/boards.png"}
    };
    
    bool texturesLoaded = rm.loadResourceBatch(debugTextures);
    if (texturesLoaded) {
        // Добавляем текстуры в уровень
        currentLevel_.wallTextures[1] = "wall_1";
        currentLevel_.floorTextures[1] = "floor_1";
        currentLevel_.ceilingTextures[1] = "ceiling_1";
        
        // Сохраняем пути
        for (const auto& [name, path] : debugTextures) {
            currentLevel_.texturePaths[name] = path;
        }
        
        std::cout << "Debug textures loaded successfully" << std::endl;
    } else {
        std::cout << "Debug textures failed to load, using color rendering" << std::endl;
    }
    
    currentLevel_.playerStartPosition = sf::Vector2f(1.5f, 1.5f);
    currentLevel_.playerStartDirection = sf::Vector2f(-1.0f, 0.0f);
    currentLevel_.backgroundMusic = "sounds/music/Adrian'sAsleep.mid";
    currentLevel_.ambienceSound = "sounds/effects/ambienceloop.wav";
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

bool Game::checkFurnitureCollision(const sf::Vector2f& position) const {
    for (const auto& obj : currentLevel_.furnitureObjects) {
        auto it = currentLevel_.furnitureTypes.find(obj.furnitureType);
        if (it != currentLevel_.furnitureTypes.end() && !it->second.passable) {
            const auto& data = it->second;
            
            // Вычисляем мировые координаты коллизионной коробки
            float boxX = obj.position.x + data.collisionBox.x - data.collisionBox.width/2;
            float boxY = obj.position.y + data.collisionBox.y - data.collisionBox.height/2;
            float boxWidth = data.collisionBox.width;
            float boxHeight = data.collisionBox.height;
            
            // Проверяем пересечение с небольшой зоной вокруг игрока
            float playerRadius = 0.2f;
            
            // Находим ближайшую точку на прямоугольнике к позиции игрока
            float closestX = std::max(boxX, std::min(position.x, boxX + boxWidth));
            float closestY = std::max(boxY, std::min(position.y, boxY + boxHeight));
            
            float distanceX = position.x - closestX;
            float distanceY = position.y - closestY;
            float distanceSquared = distanceX * distanceX + distanceY * distanceY;
            
            if (distanceSquared < (playerRadius * playerRadius)) {
                return true;
            }
        }
    }
    return false;
}