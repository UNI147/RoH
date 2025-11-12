#include "Game.h"
#include <iostream>
#include <filesystem>
#include <vector>
#include <string>

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::vector;
using std::filesystem::current_path;
using std::filesystem::exists;

Game::Game(sf::RenderWindow& window) 
    : window_(window) {
    
    // Сначала загружаем ресурсы
    loadResources();
    
    // Затем инициализируем рендерер (после загрузки текстур)
    renderer_ = std::make_unique<Renderer>(window_);
    
    // Инициализация игрока
    player_.position = sf::Vector2f(1.5f, 1.5f);
    player_.direction = sf::Vector2f(-1.0f, 0.0f);
    player_.plane = sf::Vector2f(0.0f, 0.66f);

    // Загрузка карты
    map_ = mapLoader_.createTestMap();
}

void Game::loadResources() {
    auto& rm = ResourceManager::getInstance();
    
    cout << "Current working directory: " << current_path() << endl;
    
    vector<string> possiblePaths = {
        "resources/textures/surfaces/",
        "../resources/textures/surfaces/",
        "../../resources/textures/surfaces/",
        "../../../resources/textures/surfaces/",
        "textures/surfaces/",
        "../textures/surfaces/"
    };
    
    bool texturesLoaded = false;
    
    for (const auto& path : possiblePaths) {
        string bricksPath = path + "bricks.png";
        
        if (exists(bricksPath)) {
            cout << "Found textures in: " << path << endl;
            
            if (rm.loadTexture("bricks", bricksPath) &&
                rm.loadTexture("boards", path + "boards.png") &&
                rm.loadTexture("parquet", path + "parquet.png")) {
                
                texturesLoaded = true;
                cout << "All textures successfully loaded!" << endl;
                
                try {
                    auto& bricksTex = rm.getTexture("bricks");
                    cout << "Bricks texture size: " << bricksTex.getSize().x << "x" << bricksTex.getSize().y << endl;
                } catch (const std::exception& e) {
                    cerr << "Error checking texture: " << e.what() << endl;
                }
                break;
            }
        }
    }
    
    if (!texturesLoaded) {
        cerr << "Failed to load textures from all possible paths!" << endl;
    }
}

void Game::update() {
    float deltaTime = clock_.restart().asSeconds();
    
    handleEvents();
    inputHandler_.handleInput(player_, deltaTime, map_);
}

void Game::render() {
    window_.clear();
    renderer_->renderFrame(player_, map_, rayCaster_);
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