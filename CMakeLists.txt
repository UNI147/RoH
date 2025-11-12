cmake_minimum_required(VERSION 3.10)
project(RoH)

set(CMAKE_CXX_STANDARD 17)

# Указываем путь к SFML вручную
set(SFML_DIR "C:/SFML-2.6.1/lib/cmake/SFML")
find_package(SFML 2.6.1 REQUIRED COMPONENTS graphics window system)

# Включение директорий
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

# Связывание с SFML
target_link_libraries(game sfml-graphics sfml-window sfml-system)

# Копирование DLL файлов SFML (для Windows)
if(WIN32)
    # Копируем все необходимые DLL
    get_target_property(SFML_LIB_DIR sfml-graphics LOCATION)
    get_filename_component(SFML_LIB_DIR ${SFML_LIB_DIR} DIRECTORY)
    
    add_custom_command(TARGET game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SFML_LIB_DIR}/../bin/sfml-graphics-d-2.dll"
        $<TARGET_FILE_DIR:game>
    )
    add_custom_command(TARGET game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SFML_LIB_DIR}/../bin/sfml-window-d-2.dll"
        $<TARGET_FILE_DIR:game>
    )
    add_custom_command(TARGET game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SFML_LIB_DIR}/../bin/sfml-system-d-2.dll"
        $<TARGET_FILE_DIR:game>
    )
endif()

# Копирование ресурсов в выходную директорию
if(WIN32)
    add_custom_command(TARGET game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/resources"
        $<TARGET_FILE_DIR:game>/resources
    )
else()
    # Для Linux/Mac
    add_custom_command(TARGET game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:game>/resources
    )
    add_custom_command(TARGET game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/resources/textures"
        $<TARGET_FILE_DIR:game>/resources/textures
    )
endif()