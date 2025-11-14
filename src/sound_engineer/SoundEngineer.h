#pragma once
#include <SFML/Audio.hpp>
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include <random>

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
    
    // Система фоновых звуков
    void startAmbience(const std::string& soundName, float volume = 100.0f);
    void stopAmbience();
    void setAmbienceVolume(float volume);
    
    // Система случайных звуков
    void addRandomSound(const std::string& soundName, float minDelay = 10.0f, float maxDelay = 30.0f, float volume = 100.0f);
    void clearRandomSounds();
    void updateRandomSounds(float deltaTime);
    
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
    
    // Система эха для шагов
    struct EchoSound {
        std::unique_ptr<sf::Sound> sound;
        float delay;
        float volumeMultiplier;
        float pitchMultiplier;
        float pan;
    };
    
    std::vector<EchoSound> echoSounds_;
    
    // Система инициализации аудио
    bool audioReady_ = false;
    float initializationTimer_ = 0.0f;
    float initializationProgress_ = 0.0f;
    const float REQUIRED_INIT_TIME_ = 3.0f;
    
    // Система фоновых звуков
    std::unique_ptr<sf::Sound> ambienceSound_;
    float ambienceVolume_ = 100.0f;
    
    // Система случайных звуков
    struct RandomSound {
        std::string name;
        float minDelay;
        float maxDelay;
        float volume;
        float timer;
        float nextPlayTime;
    };
    
    std::vector<RandomSound> randomSounds_;
    std::mt19937 randomGenerator_;
    
    // Вспомогательные методы
    void playStepSound(float volume, float pitch, float pan);
    void addEcho(const sf::SoundBuffer& buffer, float baseVolume, float basePitch, float pan, float delay = 0.1f);
    void updateEchoSounds(float deltaTime);
    void setupRandomSound(RandomSound& sound);
};