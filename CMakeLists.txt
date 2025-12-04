cmake_minimum_required(VERSION 3.10)
project(RoH)

set(CMAKE_CXX_STANDARD 17)

# Указываем путь к SFML
set(SFML_ROOT "C:/SFML-2.6.1")

# Функция для копирования DLL с проверкой
function(copy_dll_if_exists source dest)
    if(EXISTS "${source}")
        get_filename_component(FILENAME "${source}" NAME)
        add_custom_command(TARGET game POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                "${source}"
                "${dest}"
            COMMENT "Copying ${FILENAME}"
        )
    else()
        message(WARNING "DLL file not found: ${source}")
    endif()
endfunction()

# Добавляем пути для поиска заголовков
include_directories("${SFML_ROOT}/include")
link_directories("${SFML_ROOT}/lib")

# Включение директорий проекта
include_directories(src)
include_directories(src/engine)
include_directories(src/engine/geographer)
include_directories(src/engine/renderer)
include_directories(src/engine/raycasting)
include_directories(src/resource_manager)
include_directories(src/sound_engineer)
include_directories(src/input_handler)
include_directories(src/game/game)

# Явное указание путей vcpkg
set(VCPKG_ROOT "C:/vcpkg")
set(FLUIDSYNTH_ROOT "${VCPKG_ROOT}/installed/x64-windows")

# Проверка существования FluidSynth вручную с диагностикой
message(STATUS "Checking for FluidSynth in: ${FLUIDSYNTH_ROOT}")

if(EXISTS "${FLUIDSYNTH_ROOT}/include/fluidsynth.h")
    set(FLUIDSYNTH_INCLUDE_DIR "${FLUIDSYNTH_ROOT}/include")
    message(STATUS "Found FluidSynth includes: ${FLUIDSYNTH_INCLUDE_DIR}")
else()
    message(WARNING "FluidSynth includes not found at: ${FLUIDSYNTH_ROOT}/include/fluidsynth.h")
    set(FLUIDSYNTH_INCLUDE_DIR "")
endif()

# Поиск библиотеки FluidSynth - правильные имена
if(EXISTS "${FLUIDSYNTH_ROOT}/lib/libfluidsynth-3.lib")
    set(FLUIDSYNTH_LIBRARY "${FLUIDSYNTH_ROOT}/lib/libfluidsynth-3.lib")
    message(STATUS "Found FluidSynth library: ${FLUIDSYNTH_LIBRARY}")
elseif(EXISTS "${FLUIDSYNTH_ROOT}/lib/fluidsynth.lib")
    set(FLUIDSYNTH_LIBRARY "${FLUIDSYNTH_ROOT}/lib/fluidsynth.lib")
    message(STATUS "Found FluidSynth library: ${FLUIDSYNTH_LIBRARY}")
elseif(EXISTS "${FLUIDSYNTH_ROOT}/lib/libfluidsynth.lib")
    set(FLUIDSYNTH_LIBRARY "${FLUIDSYNTH_ROOT}/lib/libfluidsynth.lib")
    message(STATUS "Found FluidSynth library: ${FLUIDSYNTH_LIBRARY}")
else()
    message(WARNING "FluidSynth library not found in: ${FLUIDSYNTH_ROOT}/lib/")
    set(FLUIDSYNTH_LIBRARY "")
endif()

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
    src/engine/geographer/LevelProcessor.cpp
    src/engine/geographer/LevelProcessor.h
    src/engine/geographer/FurnitureLoader.cpp
    src/engine/geographer/FurnitureLoader.h
    src/engine/geographer/Furniture.h
    src/sound_engineer/SoundEngineer.cpp
    src/sound_engineer/SoundEngineer.h
    src/resource_manager/ResourceManager.cpp
    src/resource_manager/ResourceManager.h
    src/resource_manager/SoundNames.h
    src/resource_manager/FontNames.h
)

# Определение для статической линковки
target_compile_definitions(game PRIVATE SFML_STATIC)

# Обработка FluidSynth
if(FLUIDSYNTH_INCLUDE_DIR AND FLUIDSYNTH_LIBRARY)
    message(STATUS "Found FluidSynth: ${FLUIDSYNTH_LIBRARY}")
    target_include_directories(game PRIVATE ${FLUIDSYNTH_INCLUDE_DIR})
    # Добавляем FluidSynth в список библиотек для линковки
    set(ALL_LIBRARIES
        "${SFML_ROOT}/lib/sfml-graphics-s.lib"
        "${SFML_ROOT}/lib/sfml-window-s.lib"
        "${SFML_ROOT}/lib/sfml-system-s.lib"
        "${SFML_ROOT}/lib/sfml-audio-s.lib"
        ${FLUIDSYNTH_LIBRARY}
        opengl32
        freetype
        winmm
        gdi32
        openal32
        flac
        vorbisenc
        vorbisfile
        vorbis
        ogg
    )
else()
    message(WARNING "FluidSynth not found. MIDI playback will be disabled.")
    target_compile_definitions(game PRIVATE NO_FLUIDSYNTH)
    set(ALL_LIBRARIES
        "${SFML_ROOT}/lib/sfml-graphics-s.lib"
        "${SFML_ROOT}/lib/sfml-window-s.lib"
        "${SFML_ROOT}/lib/sfml-system-s.lib"
        "${SFML_ROOT}/lib/sfml-audio-s.lib"
        opengl32
        freetype
        winmm
        gdi32
        openal32
        flac
        vorbisenc
        vorbisfile
        vorbis
        ogg
    )
endif()

# ЕДИНСТВЕННЫЙ вызов target_link_libraries
target_link_libraries(game ${ALL_LIBRARIES})

# Копирование OpenAL DLL
copy_dll_if_exists("${SFML_ROOT}/bin/openal32.dll" "$<TARGET_FILE_DIR:game>")

# Для статической линковки SFML нам нужны только зависимости
set(POSSIBLE_DEPENDENCY_DLLS
    openal32.dll
)

foreach(DEP_DLL ${POSSIBLE_DEPENDENCY_DLLS})
    copy_dll_if_exists("${SFML_ROOT}/bin/${DEP_DLL}" "$<TARGET_FILE_DIR:game>")
endforeach()

# Копирование FluidSynth DLL если найден
if(FLUIDSYNTH_INCLUDE_DIR AND FLUIDSYNTH_LIBRARY)
    # Копирование DLL FluidSynth с проверкой
    set(FLUIDSYNTH_DLLS
        "libfluidsynth-3.dll"
        "libfluidsynth-2.dll" 
        "libfluidsynth.dll"
        "fluidsynth.dll"
    )
    
    foreach(FLUIDSYNTH_DLL ${FLUIDSYNTH_DLLS})
        if(EXISTS "${FLUIDSYNTH_ROOT}/bin/${FLUIDSYNTH_DLL}")
            copy_dll_if_exists("${FLUIDSYNTH_ROOT}/bin/${FLUIDSYNTH_DLL}" "$<TARGET_FILE_DIR:game>")
            message(STATUS "Copied FluidSynth DLL: ${FLUIDSYNTH_DLL}")
            break()
        endif()
    endforeach()
    
    # Также проверим debug директорию
    foreach(FLUIDSYNTH_DLL ${FLUIDSYNTH_DLLS})
        if(EXISTS "${FLUIDSYNTH_ROOT}/debug/bin/${FLUIDSYNTH_DLL}")
            copy_dll_if_exists("${FLUIDSYNTH_ROOT}/debug/bin/${FLUIDSYNTH_DLL}" "$<TARGET_FILE_DIR:game>")
            message(STATUS "Copied FluidSynth DLL from debug: ${FLUIDSYNTH_DLL}")
            break()
        endif()
    endforeach()
    
    # КОПИРОВАНИЕ ЗАВИСИМОСТЕЙ FLUIDSYNTH
    message(STATUS "Looking for FluidSynth dependencies...")
    
    # Полный список зависимостей GLib и FluidSynth
    set(FLUIDSYNTH_DEPENDENCIES
        # GLib основные
        "glib-2.0-0.dll"
        "gobject-2.0-0.dll"
        "gio-2.0-0.dll"
        "gmodule-2.0-0.dll"
        "gthread-2.0-0.dll"
        
        # Internationalization
        "intl.dll"
        "iconv.dll"
        "libiconv-2.dll"
        "libintl-8.dll"
        "libintl-9.dll"
        "libintl.dll"
        
        # PCRE
        "pcre2-8.dll"
        "pcre2-16.dll"
        "pcre2-32.dll"
        "pcre-1.dll"
        "pcre.dll"
        
        # Другие системные зависимости
        "libwinpthread-1.dll"
        "zlib1.dll"
        "libffi-7.dll"
        "libffi-8.dll"
        "ffi.dll"
    )
    
    # Ищем в bin директориях vcpkg
    foreach(DEP_DLL ${FLUIDSYNTH_DEPENDENCIES})
        if(EXISTS "${FLUIDSYNTH_ROOT}/bin/${DEP_DLL}")
            copy_dll_if_exists("${FLUIDSYNTH_ROOT}/bin/${DEP_DLL}" "$<TARGET_FILE_DIR:game>")
            message(STATUS "Copied FluidSynth dependency: ${DEP_DLL}")
        elseif(EXISTS "${FLUIDSYNTH_ROOT}/debug/bin/${DEP_DLL}")
            copy_dll_if_exists("${FLUIDSYNTH_ROOT}/debug/bin/${DEP_DLL}" "$<TARGET_FILE_DIR:game>")
            message(STATUS "Copied FluidSynth dependency from debug: ${DEP_DLL}")
        else()
            message(STATUS "Dependency not found in fluidsynth dir: ${DEP_DLL}")
        endif()
    endforeach()
    
    # Дополнительно ищем в корне установки vcpkg
    foreach(DEP_DLL ${FLUIDSYNTH_DEPENDENCIES})
        if(EXISTS "${VCPKG_ROOT}/installed/x64-windows/bin/${DEP_DLL}")
            copy_dll_if_exists("${VCPKG_ROOT}/installed/x64-windows/bin/${DEP_DLL}" "$<TARGET_FILE_DIR:game>")
            message(STATUS "Copied dependency from vcpkg bin: ${DEP_DLL}")
        endif()
    endforeach()
    
    # РУЧНОЕ КОПИРОВАНИЕ КРИТИЧЕСКИХ ЗАВИСИМОСТЕЙ
    # Эти библиотеки часто находятся в разных местах
    set(CRITICAL_DEPENDENCIES
        "${VCPKG_ROOT}/installed/x64-windows/bin/libintl-8.dll"
        "${VCPKG_ROOT}/installed/x64-windows/bin/libiconv-2.dll"
        "${VCPKG_ROOT}/installed/x64-windows/tools/glib/bin/libintl-8.dll"
        "${VCPKG_ROOT}/installed/x64-windows/tools/glib/bin/libiconv-2.dll"
        "${VCPKG_ROOT}/packages/libiconv_x64-windows/bin/libiconv-2.dll"
        "${VCPKG_ROOT}/packages/libintl_x64-windows/bin/libintl-8.dll"
    )
    
    foreach(CRITICAL_DLL ${CRITICAL_DEPENDENCIES})
        if(EXISTS "${CRITICAL_DLL}")
            copy_dll_if_exists("${CRITICAL_DLL}" "$<TARGET_FILE_DIR:game>")
            message(STATUS "Copied critical dependency: ${CRITICAL_DLL}")
        endif()
    endforeach()
    
    # Альтернативный способ: найти все DLL рекурсивно в vcpkg
    file(GLOB_RECURSE ALL_VCPKG_DLLS 
        "${VCPKG_ROOT}/installed/x64-windows/*.dll"
        "${VCPKG_ROOT}/packages/*/bin/*.dll"
    )
    
    # Копируем только нужные DLL
    foreach(VCPKG_DLL ${ALL_VCPKG_DLLS})
        get_filename_component(DLL_NAME "${VCPKG_DLL}" NAME)
        foreach(PATTERN 
            "libintl*" "libiconv*" "intl*" "iconv*" 
            "glib*" "gobject*" "gio*" "gmodule*" "gthread*"
            "pcre*" "ffi*" "zlib*" "winpthread*"
        )
            if(DLL_NAME MATCHES "${PATTERN}")
                copy_dll_if_exists("${VCPKG_DLL}" "$<TARGET_FILE_DIR:game>")
                message(STATUS "Copied matched DLL: ${DLL_NAME}")
                break()
            endif()
        endforeach()
    endforeach()
endif()

# Копирование ресурсов
add_custom_command(TARGET game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/resources"
        "$<TARGET_FILE_DIR:game>/resources"
    COMMENT "Copying resources"
)