cmake_minimum_required(VERSION 3.10)
project(RoH)

set(CMAKE_CXX_STANDARD 17)

# Указываем путь к SFML
set(SFML_ROOT "C:/SFML-2.6.1")

# Добавляем пути для поиска заголовков
include_directories("${SFML_ROOT}/include")
link_directories("${SFML_ROOT}/lib")

# Включение директорий проекта
include_directories(src)

# Сборка исполняемого файла
add_executable(game
    src/main.cpp
    src/game/game/Game.cpp
    src/game/game/Game.h
    src/input_handler/InputHandler.cpp
    src/input_handler/InputHandler.h
    src/engine/raycasting/RayCaster.cpp
    src/engine/raycasting/RayCaster.h
    src/engine/renderer/Renderer.cpp
    src/engine/renderer/Renderer.h
    src/engine/geographer/MapLoader.cpp
    src/engine/geographer/MapLoader.h
    src/resource_manager/ResourceManager.cpp
    src/resource_manager/ResourceManager.h
    src/resource_manager/TextureNames.h
)

# Статическая линковка SFML библиотек
target_link_libraries(game 
    "${SFML_ROOT}/lib/sfml-graphics-s.lib"
    "${SFML_ROOT}/lib/sfml-window-s.lib"
    "${SFML_ROOT}/lib/sfml-system-s.lib"
    opengl32
    freetype
    winmm
    gdi32
)

# Определение для статической линковки
target_compile_definitions(game PRIVATE SFML_STATIC)

# Копирование ресурсов
add_custom_command(TARGET game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/resources"
        "$<TARGET_FILE_DIR:game>/resources"
)