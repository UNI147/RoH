#include "SoundEngineer.h"
#include <iostream>
#include <algorithm>
#include "resource_manager/ResourceManager.h"
#include "resource_manager/MusicNames.h"
#include "resource_manager/SoundNames.h"

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#endif

namespace FluidSynthUtils {
    // Подавление логов через callback
    static bool fluidSynthInitialized = false;
    
    static void fluidSynthLogger(int level, const char* message, void* data) {
        // Игнорируем все сообщения, но не перенаправляем stderr
    }
    
    static void setupFluidSynthLogging() {
        if (!fluidSynthInitialized) {
            fluid_set_log_function(FLUID_PANIC, &fluidSynthLogger, nullptr);
            fluid_set_log_function(FLUID_ERR, &fluidSynthLogger, nullptr);
            fluid_set_log_function(FLUID_WARN, &fluidSynthLogger, nullptr);
            fluid_set_log_function(FLUID_INFO, &fluidSynthLogger, nullptr);
            fluid_set_log_function(FLUID_DBG, &fluidSynthLogger, nullptr);
            fluidSynthInitialized = true;
        }
    }
};

// Конструктор SoundEngineer
SoundEngineer::SoundEngineer() {
#ifndef NO_FLUIDSYNTH
    settings_ = nullptr;
    synth_ = nullptr;
    player_ = nullptr;
    driver_ = nullptr;
#endif
}

// Деструктор SoundEngineer
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

bool SoundEngineer::initializeFluidSynth(const std::string& soundFontName) {
#ifndef NO_FLUIDSYNTH
    // Сбрасываем состояние инициализации
    audioReady_ = false;
    initializationTimer_ = 0.0f;
    initializationProgress_ = 0.0f;
    
    // Настройка логирования через callback
    FluidSynthUtils::setupFluidSynthLogging();
    
    settings_ = new_fluid_settings();
    if (!settings_) {
        std::cerr << "Failed to create FluidSynth settings" << std::endl;
        return false;
    }
    
    // КРИТИЧЕСКИ ВАЖНЫЕ НАСТРОЙКИ для качества звука
    fluid_settings_setint(settings_, "synth.threadsafe-api", 1);
    fluid_settings_setint(settings_, "synth.lock-memory", 0);
    
    // Настройки аудиобуфера
    fluid_settings_setint(settings_, "audio.periods", 8);
    fluid_settings_setint(settings_, "audio.period-size", 1024);
    
    // Частота дискретизации - устанавливаем ТОЛЬКО через settings
    fluid_settings_setint(settings_, "synth.sample-rate", 44100);
    
    // КАЧЕСТВО СИНТЕЗА
    fluid_settings_setnum(settings_, "synth.gain", 0.7);
    fluid_settings_setint(settings_, "synth.polyphony", 256);
    
    // Настройки реверберации
    fluid_settings_setnum(settings_, "synth.reverb.room-size", 0.4);
    fluid_settings_setnum(settings_, "synth.reverb.damp", 0.3);
    fluid_settings_setnum(settings_, "synth.reverb.width", 0.7);
    fluid_settings_setnum(settings_, "synth.reverb.level", 0.2);
    
    // Настройки хора
    fluid_settings_setnum(settings_, "synth.chorus.level", 0.1);
    fluid_settings_setnum(settings_, "synth.chorus.depth", 4.0);
    fluid_settings_setnum(settings_, "synth.chorus.speed", 0.2);
    fluid_settings_setint(settings_, "synth.chorus.nr", 3);
    
    // Включаем эффекты
    fluid_settings_setint(settings_, "synth.reverb.active", 1);
    fluid_settings_setint(settings_, "synth.chorus.active", 1);
    
    // Отключаем verbose режим
    fluid_settings_setint(settings_, "synth.verbose", 0);
    fluid_settings_setint(settings_, "synth.dump", 0);
    fluid_settings_setint(settings_, "synth.report-midi-errors", 0);
    fluid_settings_setint(settings_, "audio.verbose", 0);
    
    // Настройки для Windows
    #ifdef _WIN32
    fluid_settings_setstr(settings_, "audio.driver", "dsound");
    fluid_settings_setint(settings_, "audio.dsound.periods", 8);
    #else
    fluid_settings_setstr(settings_, "audio.driver", "alsa");
    #endif
    
    synth_ = new_fluid_synth(settings_);
    if (!synth_) {
        std::cerr << "Failed to create FluidSynth synthesizer" << std::endl;
        delete_fluid_settings(settings_);
        settings_ = nullptr;
        return false;
    }
    
    // ВЫСОКОКАЧЕСТВЕННАЯ ИНТЕРПОЛЯЦИЯ
    fluid_synth_set_interp_method(synth_, -1, FLUID_INTERP_HIGHEST);
    
    // Устанавливаем полифонию
    fluid_synth_set_polyphony(synth_, 256);
    
    // Загрузка SoundFont
    auto& rm = ResourceManager::getInstance();
    if (rm.hasSoundFont(soundFontName)) {
        try {
            const std::string& soundFontPath = rm.getSoundFontPath(soundFontName);
            int fontId = fluid_synth_sfload(synth_, soundFontPath.c_str(), 1);
            if (fontId != -1) {
                std::cout << "Successfully loaded SoundFont: " << soundFontPath << " (ID: " << fontId << ")" << std::endl;
                
                // Предварительная инициализация синтезатора
                fluid_synth_program_select(synth_, 0, fontId, 0, 0);
                
                // УСИЛЕНИЕ
                fluid_synth_set_gain(synth_, 0.5f);
                
                // Предварительный рендеринг для прогрева
                fluid_synth_noteon(synth_, 0, 60, 80);
                fluid_synth_noteoff(synth_, 0, 60);
                
            } else {
                std::cerr << "Failed to load SoundFont file: " << soundFontPath << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error loading SoundFont: " << e.what() << std::endl;
        }
    } else {
        std::cerr << "SoundFont not found in ResourceManager: " << soundFontName << std::endl;
    }
    
    // Создаем аудиодрайвер
    driver_ = new_fluid_audio_driver(settings_, synth_);
    if (!driver_) {
        std::cerr << "Failed to create FluidSynth audio driver, trying default..." << std::endl;
        #ifdef _WIN32
        fluid_settings_setstr(settings_, "audio.driver", "waveout");
        #else
        fluid_settings_setstr(settings_, "audio.driver", "pulseaudio");
        #endif
        driver_ = new_fluid_audio_driver(settings_, synth_);
    }
    
    if (driver_) {
        std::cout << "FluidSynth initialized successfully with audio driver" << std::endl;
        
        initializationTimer_ = 0.0f;
        initializationProgress_ = 0.0f;
        audioReady_ = false;
        
        return true;
    } else {
        std::cout << "FluidSynth initialized without audio driver" << std::endl;
        audioReady_ = true;
        initializationProgress_ = 1.0f;
        return false;
    }
#else
    std::cout << "FluidSynth compilation disabled" << std::endl;
    audioReady_ = true;
    initializationProgress_ = 1.0f;
    return false;
#endif
}

void SoundEngineer::setMIDIVolume(float volume) {
#ifndef NO_FLUIDSYNTH
    if (synth_) {
        // volume от 0.0 до 1.0
        float safeVolume = std::min(volume, 1.0f);
        fluid_synth_set_gain(synth_, safeVolume);
    }
#endif
}

void SoundEngineer::updateInitialization(float deltaTime) {
    if (audioReady_) return;
    
    initializationTimer_ += deltaTime;
    initializationProgress_ = initializationTimer_ / REQUIRED_INIT_TIME_;
    
    if (initializationTimer_ >= REQUIRED_INIT_TIME_) {
        audioReady_ = true;
        initializationProgress_ = 1.0f;
        std::cout << "Audio system fully initialized and ready!" << std::endl;
        
        // Финальная настройка после инициализации
        #ifndef NO_FLUIDSYNTH
        if (synth_) {
            // Сбрасываем все ноты на всякий случай
            fluid_synth_system_reset(synth_);
        }
        #endif
    }
}

void SoundEngineer::playMIDI(const std::string& name, bool loop) {
#ifndef NO_FLUIDSYNTH
    if (!synth_) {
        std::cerr << "Synth not initialized, cannot play MIDI: " << name << std::endl;
        return;
    }
    
    auto& rm = ResourceManager::getInstance();
    if (!rm.hasMIDI(name)) {
        std::cerr << "MIDI not found in ResourceManager: " << name << std::endl;
        return;
    }
    
    stopMIDI();
    
    try {
        const std::string& filename = rm.getMIDIPath(name);
        
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
            fluid_player_set_loop(player_, -1);
        }
        
        fluid_player_play(player_);
        currentMIDI_ = name;
        
        std::cout << "Playing MIDI: " << filename << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error playing MIDI: " << e.what() << std::endl;
    }
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
    } else if (rm.hasMIDI(name)) {
        // Воспроизводим как MIDI, если это MIDI файл
        playMIDI(name, loop);
    } else {
        std::cerr << "Music not found: " << name << std::endl;
    }
}

void SoundEngineer::stopMusic() {
    // Останавливаем и MIDI и SFML музыку
    stopMIDI();
    
    // Здесь можно добавить остановку SFML музыки
}

void SoundEngineer::setMusicVolume(float volume) {
    // Устанавливаем громкость для MIDI
    setMIDIVolume(volume / 100.0f);
    
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