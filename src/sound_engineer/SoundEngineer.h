#pragma once
#include <SFML/Audio.hpp>
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

#ifndef NO_FLUIDSYNTH
#include <fluidsynth.h>
#endif

class SoundEngineer {
public:
    SoundEngineer();
    ~SoundEngineer();
    
    // Инициализация FluidSynth с SoundFont
    bool initializeFluidSynth(const std::string& soundFontName = "default");
    
    // MIDI методы
    void playMIDI(const std::string& name, bool loop = false);
    void stopMIDI();
    void setMIDIVolume(float volume);
    
    // Музыка методы
    void playMusic(const std::string& name, bool loop = false);
    void stopMusic();
    void setMusicVolume(float volume);
    
    // Звуковые эффекты
    void playSound(const std::string& name, float volume = 100.0f, float pitch = 1.0f);
    void stopAllSounds();
    
    // Система шагов
    void updateFootsteps(bool isMoving, bool isRunning, float deltaTime);
    void setFootstepsEnabled(bool enabled);
    
    // Методы для проверки готовности
    bool isAudioReady() const { return audioReady_; }
    float getInitializationProgress() const { return initializationProgress_; }
    void updateInitialization(float deltaTime);
    
private:
#ifndef NO_FLUIDSYNTH
    fluid_settings_t* settings_;
    fluid_synth_t* synth_;
    fluid_player_t* player_;
    fluid_audio_driver_t* driver_;
#endif
    std::string currentMIDI_;
    
    // Система звуковых эффектов
    std::vector<std::unique_ptr<sf::Sound>> activeSounds_;
    
    // Система шагов
    bool footstepsEnabled_ = true;
    float stepTimer_ = 0.0f;
    bool leftStep_ = true;
    float baseStepInterval_ = 0.5f;
    
    // Система инициализации аудио
    bool audioReady_ = false;
    float initializationTimer_ = 0.0f;
    float initializationProgress_ = 0.0f;
    const float REQUIRED_INIT_TIME_ = 3.0f;
    
    // Создание звука с эхом
    void playStepSound(float volume, float pitch);
};