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
    src/input_handler/InputHandler.cpp
    src/engine/raycasting/RayCaster.cpp
    src/engine/renderer/Renderer.cpp
    src/engine/geographer/MapLoader.cpp
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