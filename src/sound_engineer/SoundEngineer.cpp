#define _CRT_SECURE_NO_WARNINGS
#include "SoundEngineer.h"
#include <iostream>
#include <algorithm>
#include <random>
#include "resource_manager/ResourceManager.h"
#include "resource_manager/SoundNames.h"

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
SoundEngineer::SoundEngineer() 
    : randomGenerator_(std::random_device{}()) {
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

// ДОБАВЛЯЕМ НЕДОСТАЮЩИЙ МЕТОД
void SoundEngineer::setupRandomSound(RandomSound& sound) {
    std::uniform_real_distribution<float> dist(sound.minDelay, sound.maxDelay);
    sound.nextPlayTime = dist(randomGenerator_);
    sound.timer = 0.0f;
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
            std::cout << "Attempting to load SoundFont from: " << soundFontPath << std::endl;
            
            int fontId = fluid_synth_sfload(synth_, soundFontPath.c_str(), 1);
            if (fontId != -1) {
                std::cout << "Successfully loaded SoundFont with ID: " << fontId << std::endl;
            } else {
                std::cerr << "FluidSynth failed to load SoundFont file: " << soundFontPath << std::endl;
                // Проверим права доступа к файлу
                FILE* test = fopen(soundFontPath.c_str(), "rb");
                if (test) {
                    std::cout << "File exists and is readable" << std::endl;
                    fclose(test);
                } else {
                    std::cerr << "Cannot open SoundFont file for reading" << std::endl;
                }
                return false;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error loading SoundFont: " << e.what() << std::endl;
            return false;
        }
    } else {
        std::cerr << "SoundFont not registered in ResourceManager: " << soundFontName << std::endl;
        return false;
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

// Система фоновых звуков
void SoundEngineer::startAmbience(const std::string& soundName, float volume) {
    auto& rm = ResourceManager::getInstance();
    
    if (rm.hasSound(soundName)) {
        try {
            ambienceSound_ = std::make_unique<sf::Sound>();
            ambienceSound_->setBuffer(rm.getSound(soundName));
            ambienceSound_->setLoop(true);
            ambienceSound_->setVolume(volume);
            ambienceSound_->play();
            ambienceVolume_ = volume;
            
            std::cout << "Started ambience sound: " << soundName << " at volume " << volume << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error starting ambience sound: " << e.what() << std::endl;
        }
    } else {
        std::cerr << "Ambience sound not found: " << soundName << std::endl;
    }
}

void SoundEngineer::stopAmbience() {
    if (ambienceSound_) {
        ambienceSound_->stop();
        ambienceSound_.reset();
    }
}

void SoundEngineer::setAmbienceVolume(float volume) {
    ambienceVolume_ = volume;
    if (ambienceSound_) {
        ambienceSound_->setVolume(volume);
    }
}

// Система случайных звуков
void SoundEngineer::addRandomSound(const std::string& soundName, float minDelay, float maxDelay, float volume) {
    RandomSound newSound;
    newSound.name = soundName;
    newSound.minDelay = minDelay;
    newSound.maxDelay = maxDelay;
    newSound.volume = volume;
    
    setupRandomSound(newSound);
    randomSounds_.push_back(newSound);
    
    std::cout << "Added random sound: " << soundName 
              << " with delay " << minDelay << "-" << maxDelay 
              << "s, volume " << volume << std::endl;
}

void SoundEngineer::clearRandomSounds() {
    randomSounds_.clear();
}

void SoundEngineer::updateRandomSounds(float deltaTime) {
    for (auto& sound : randomSounds_) {
        sound.timer += deltaTime;
        
        if (sound.timer >= sound.nextPlayTime) {
            // Воспроизводим звук
            playSound(sound.name, sound.volume, 1.0f);
            
            // Сбрасываем таймер и устанавливаем следующее время воспроизведения
            sound.timer = 0.0f;
            setupRandomSound(sound);
        }
    }
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
    
    // Обновляем случайные звуки даже во время инициализации
    updateRandomSounds(deltaTime);
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
    
    for (auto& echo : echoSounds_) {
        echo.sound->stop();
    }
    echoSounds_.clear();
}

// Система шагов
void SoundEngineer::updateFootsteps(bool isMoving, bool isRunning, float deltaTime) {
    if (!footstepsEnabled_) return;
    
    // Обновляем систему эха
    updateEchoSounds(deltaTime);
    
    if (isMoving) {
        stepTimer_ -= deltaTime;
        
        if (stepTimer_ <= 0.0f) {
            // Определяем интервал между шагами
            float stepInterval = isRunning ? baseStepInterval_ * 0.6f : baseStepInterval_;
            
            // Параметры в зависимости от типа движения
            float volume, pitch, pan;
            
            if (isRunning) {
                volume = leftStep_ ? 85.0f : 75.0f;
                pitch = leftStep_ ? 1.15f : 1.1f;
                pan = leftStep_ ? -0.4f : 0.4f;
            } else {
                volume = leftStep_ ? 65.0f : 55.0f;
                pitch = leftStep_ ? 0.95f : 1.0f;
                pan = leftStep_ ? -0.2f : 0.2f;
            }
            
            // Воспроизводим звук шага с правильным панорамированием
            playStepSound(volume, pitch, pan);
            
            // Чередуем ноги
            leftStep_ = !leftStep_;
            
            // Сбрасываем таймер
            stepTimer_ = stepInterval;
        }
    } else {
        stepTimer_ = 0.0f;
        // Сбрасываем чередование ног при остановке
        leftStep_ = true;
    }
    
    // Очищаем завершенные звуки (кроме ambience и echo)
    activeSounds_.erase(
        std::remove_if(activeSounds_.begin(), activeSounds_.end(),
            [](const std::unique_ptr<sf::Sound>& sound) {
                return sound->getStatus() == sf::Sound::Stopped;
            }),
        activeSounds_.end()
    );
    
    // Обновляем случайные звуки
    updateRandomSounds(deltaTime);
}

// Реализация playStepSound с панорамированием и эхом
void SoundEngineer::playStepSound(float volume, float pitch, float pan) {
    auto& rm = ResourceManager::getInstance();
    
    if (rm.hasSound(Sounds::STEP)) {
        try {
            // Основной звук шага
            auto mainSound = std::make_unique<sf::Sound>();
            mainSound->setBuffer(rm.getSound(Sounds::STEP));
            mainSound->setVolume(volume);
            mainSound->setPitch(pitch);
            
            // Устанавливаем панорамирование
            #if SFML_VERSION_MAJOR >= 2 && SFML_VERSION_MINOR >= 5
            mainSound->setPosition(pan, 0.0f, 0.0f);
            mainSound->setMinDistance(1.0f);
            mainSound->setAttenuation(0.5f);
            #else
            // Альтернатива для старых версий SFML
            mainSound->setPosition(pan * 10.0f, 0.0f, 0.0f);
            #endif
            
            mainSound->setRelativeToListener(true);
            mainSound->play();
            activeSounds_.push_back(std::move(mainSound));
            
            // Добавляем эхо-эффект с задержкой
            addEcho(rm.getSound(Sounds::STEP), volume * 0.3f, pitch * 0.8f, pan, 0.15f);
            
            // Второе, более тихое эхо
            addEcho(rm.getSound(Sounds::STEP), volume * 0.15f, pitch * 0.7f, pan, 0.3f);
            
        } catch (const std::exception& e) {
            std::cerr << "Error playing step sound: " << e.what() << std::endl;
        }
    } else {
        std::cerr << "Step sound not found: " << Sounds::STEP << std::endl;
    }
}

// Метод для добавления эха
void SoundEngineer::addEcho(const sf::SoundBuffer& buffer, float baseVolume, float basePitch, float pan, float delay) {
    EchoSound echo;
    echo.sound = std::make_unique<sf::Sound>();
    echo.sound->setBuffer(buffer);
    echo.sound->setVolume(0.0f);
    echo.sound->setPitch(basePitch);
    echo.delay = delay;
    echo.volumeMultiplier = baseVolume;
    echo.pitchMultiplier = basePitch;
    echo.pan = pan;
    
    // Настраиваем позиционирование
    #if SFML_VERSION_MAJOR >= 2 && SFML_VERSION_MINOR >= 5
    echo.sound->setPosition(pan, 0.0f, 0.0f);
    echo.sound->setMinDistance(1.0f);
    echo.sound->setAttenuation(0.3f);
    #else
    echo.sound->setPosition(pan * 10.0f, 0.0f, 0.0f);
    #endif
    
    echo.sound->setRelativeToListener(true);
    echo.sound->play();
    
    echoSounds_.push_back(std::move(echo));
}

// Метод для обновления системы эха
void SoundEngineer::updateEchoSounds(float deltaTime) {
    for (auto it = echoSounds_.begin(); it != echoSounds_.end(); ) {
        auto& echo = *it;
        
        if (echo.sound->getStatus() == sf::Sound::Stopped) {
            it = echoSounds_.erase(it);
            continue;
        }
        
        // Уменьшаем задержку
        echo.delay -= deltaTime;
        
        if (echo.delay <= 0.0f) {
            // Когда задержка прошла, устанавливаем полную громкость
            echo.sound->setVolume(echo.volumeMultiplier);
        } else if (echo.delay < 0.1f) {
            // Плавное нарастание громкости в последние 0.1 секунды
            float fadeIn = 1.0f - (echo.delay / 0.1f);
            echo.sound->setVolume(echo.volumeMultiplier * fadeIn);
        }
        
        ++it;
    }
}