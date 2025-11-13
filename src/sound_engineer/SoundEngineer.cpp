#include "SoundEngineer.h"
#include <iostream>
#include <algorithm>
#include "resource_manager/ResourceManager.h"
#include "resource_manager/MusicNames.h"
#include "resource_manager/SoundNames.h"

SoundEngineer::SoundEngineer() 
#ifndef NO_FLUIDSYNTH
    : settings_(nullptr), synth_(nullptr), player_(nullptr), driver_(nullptr)
#endif
{
#ifndef NO_FLUIDSYNTH
    std::cout << "Initializing FluidSynth..." << std::endl;
    
    settings_ = new_fluid_settings();
    if (!settings_) {
        std::cerr << "Failed to create FluidSynth settings" << std::endl;
        return;
    }
    
    // Настройки для Windows
    fluid_settings_setstr(settings_, "audio.driver", "dsound");
    fluid_settings_setint(settings_, "synth.sample-rate", 44100);
    
    synth_ = new_fluid_synth(settings_);
    if (!synth_) {
        std::cerr << "Failed to create FluidSynth synthesizer" << std::endl;
        delete_fluid_settings(settings_);
        settings_ = nullptr;
        return;
    }
    
    // Загрузка SoundFont
    std::vector<std::string> soundfontPaths = {
        "resources/OPL3SB.sf2",
        "../resources/OPL3SB.sf2", 
        "../../resources/OPL3SB.sf2",
        "OPL3SB.sf2"
    };
    
    bool soundfontLoaded = false;
    for (const auto& path : soundfontPaths) {
        if (fluid_synth_sfload(synth_, path.c_str(), 1) != -1) {
            std::cout << "Successfully loaded SoundFont: " << path << std::endl;
            soundfontLoaded = true;
            break;
        }
    }
    
    if (!soundfontLoaded) {
        std::cerr << "Failed to load any SoundFont file" << std::endl;
        // Не прерываем инициализацию - может работать без SoundFont
    }
    
    driver_ = new_fluid_audio_driver(settings_, synth_);
    if (!driver_) {
        std::cerr << "Failed to create FluidSynth audio driver, trying default..." << std::endl;
        // Пробуем с драйвером по умолчанию
        fluid_settings_setstr(settings_, "audio.driver", "");
        driver_ = new_fluid_audio_driver(settings_, synth_);
    }
    
    if (driver_) {
        std::cout << "FluidSynth initialized successfully with audio driver" << std::endl;
    } else {
        std::cout << "FluidSynth initialized without audio driver" << std::endl;
    }
#else
    std::cout << "FluidSynth compilation disabled" << std::endl;
#endif
}

SoundEngineer::~SoundEngineer() {
#ifndef NO_FLUIDSYNTH
    if (player_) {
        delete_fluid_player(player_);
        player_ = nullptr;
    }
    if (driver_) {
        delete_fluid_audio_driver(driver_);
        driver_ = nullptr;
    }
    if (synth_) {
        delete_fluid_synth(synth_);
        synth_ = nullptr;
    }
    if (settings_) {
        delete_fluid_settings(settings_);
        settings_ = nullptr;
    }
#endif
}

bool SoundEngineer::loadMIDI(const std::string& name, const std::string& filename) {
#ifndef NO_FLUIDSYNTH
    if (!synth_) {
        std::cerr << "Synth not initialized for MIDI: " << name << std::endl;
        return false;
    }
    
    // Проверяем существование файла безопасным способом
    FILE* testFile = nullptr;
    errno_t err = fopen_s(&testFile, filename.c_str(), "rb");
    if (err != 0 || !testFile) {
        std::cerr << "MIDI file not found: " << filename << " (error: " << err << ")" << std::endl;
        return false;
    }
    fclose(testFile);
    
    currentMIDI_ = name;
    return true;
#else
    std::cout << "MIDI loading disabled - FluidSynth not available" << std::endl;
    return false;
#endif
}

void SoundEngineer::playMIDI(const std::string& name, bool loop) {
#ifndef NO_FLUIDSYNTH
    if (!synth_) {
        std::cerr << "Synth not initialized, cannot play MIDI: " << name << std::endl;
        return;
    }
    
    stopMIDI();
    
    auto& rm = ResourceManager::getInstance();
    std::string filename;
    
    // Пытаемся получить путь из ResourceManager или используем прямое имя
    try {
        filename = "resources/sounds/music/" + name + ".mid";
    } catch (...) {
        filename = name;
    }
    
    player_ = new_fluid_player(synth_);
    if (!player_) {
        std::cerr << "Failed to create FluidSynth player" << std::endl;
        return;
    }
    
    int result = fluid_player_add(player_, filename.c_str());
    if (result != FLUID_OK) {
        std::cerr << "Failed to add MIDI file to player: " << filename << std::endl;
        delete_fluid_player(player_);
        player_ = nullptr;
        return;
    }
    
    if (loop) {
        fluid_player_set_loop(player_, -1); // Бесконечный цикл
    }
    
    fluid_player_play(player_);
    currentMIDI_ = name;
    
    std::cout << "Playing MIDI: " << filename << std::endl;
#else
    std::cout << "MIDI playback disabled - FluidSynth not available" << std::endl;
#endif
}

void SoundEngineer::stopMIDI() {
#ifndef NO_FLUIDSYNTH
    if (player_) {
        fluid_player_stop(player_);
        fluid_player_join(player_);
        delete_fluid_player(player_);
        player_ = nullptr;
        currentMIDI_.clear();
    }
#endif
}

void SoundEngineer::setMIDIVolume(float volume) {
#ifndef NO_FLUIDSYNTH
    if (synth_) {
        // volume от 0.0 до 1.0
        fluid_synth_set_gain(synth_, volume);
    }
#endif
}

// Реализация методов музыки
void SoundEngineer::playMusic(const std::string& name, bool loop) {
    auto& rm = ResourceManager::getInstance();
    if (rm.hasMusic(name)) {
        try {
            sf::Music& music = rm.getMusic(name);
            music.setLoop(loop);
            music.play();
            std::cout << "Playing SFML music: " << name << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error playing SFML music: " << e.what() << std::endl;
        }
    } else {
        // Пытаемся воспроизвести как MIDI, если это MIDI файл
        if (name.find(".mid") != std::string::npos) {
            playMIDI(name, loop);
        } else {
            std::cerr << "Music not found: " << name << std::endl;
        }
    }
}

void SoundEngineer::stopMusic() {
    // Останавливаем и MIDI и SFML музыку
    stopMIDI();
    
    // Здесь можно добавить остановку SFML музыки
}

void SoundEngineer::setMusicVolume(float volume) {
    // Устанавливаем громкость для MIDI
    setMIDIVolume(volume / 100.0f); // Конвертируем из 0-100 в 0.0-1.0
    
    // Здесь можно добавить установку громкости для SFML музыки
}

// Звуковые эффекты
void SoundEngineer::playSound(const std::string& name, float volume, float pitch) {
    auto& rm = ResourceManager::getInstance();
    if (rm.hasSound(name)) {
        try {
            auto sound = std::make_unique<sf::Sound>();
            sound->setBuffer(rm.getSound(name));
            sound->setVolume(volume);
            sound->setPitch(pitch);
            sound->play();
            
            // Добавляем в активные звуки
            activeSounds_.push_back(std::move(sound));
        } catch (const std::exception& e) {
            std::cerr << "Error playing sound: " << e.what() << std::endl;
        }
    } else {
        std::cerr << "Sound not found: " << name << std::endl;
    }
}

void SoundEngineer::stopAllSounds() {
    for (auto& sound : activeSounds_) {
        sound->stop();
    }
    activeSounds_.clear();
}

// Система шагов
void SoundEngineer::updateFootsteps(bool isMoving, bool isRunning, float deltaTime) {
    if (!footstepsEnabled_) return;
    
    if (isMoving) {
        stepTimer_ -= deltaTime;
        
        if (stepTimer_ <= 0.0f) {
            // Определяем интервал между шагами
            float stepInterval = baseStepInterval_;
            if (isRunning) {
                stepInterval *= 0.6f;
            }
            
            // Воспроизводим звук шага
            playStepSound(leftStep_ ? 100.0f : 50.0f, isRunning ? 1.2f : 1.0f);
            
            // Чередуем ноги
            leftStep_ = !leftStep_;
            
            // Сбрасываем таймер
            stepTimer_ = stepInterval;
        }
    } else {
        stepTimer_ = 0.0f;
    }
    
    // Очищаем завершенные звуки
    activeSounds_.erase(
        std::remove_if(activeSounds_.begin(), activeSounds_.end(),
            [](const std::unique_ptr<sf::Sound>& sound) {
                return sound->getStatus() == sf::Sound::Stopped;
            }),
        activeSounds_.end()
    );
}

void SoundEngineer::setFootstepsEnabled(bool enabled) {
    footstepsEnabled_ = enabled;
    if (!enabled) {
        stepTimer_ = 0.0f;
    }
}

void SoundEngineer::playStepSound(float volume, float pitch) {
    auto& rm = ResourceManager::getInstance();
    
    // Основной звук шага
    if (rm.hasSound(Sounds::STEP)) {
        auto mainSound = std::make_unique<sf::Sound>();
        mainSound->setBuffer(rm.getSound(Sounds::STEP));
        mainSound->setVolume(volume);
        mainSound->setPitch(pitch);
        mainSound->play();
        activeSounds_.push_back(std::move(mainSound));
        
        // Эхо-эффект (воспроизводим с небольшой задержкой)
        auto echoSound = std::make_unique<sf::Sound>();
        echoSound->setBuffer(rm.getSound(Sounds::STEP));
        echoSound->setVolume(volume * 0.5f);
        echoSound->setPitch(pitch * 0.75f);
        echoSound->setPlayingOffset(sf::milliseconds(80));
        echoSound->play();
        activeSounds_.push_back(std::move(echoSound));
    }
}