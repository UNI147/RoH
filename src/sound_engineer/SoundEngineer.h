#pragma once
#include <SFML/Audio.hpp>
#include <string>
#include <unordered_map>
#include <memory>

// Условная компиляция для FluidSynth
#ifndef NO_FLUIDSYNTH
#include <fluidsynth.h>
#endif

class SoundEngineer {
public:
    SoundEngineer();
    ~SoundEngineer();
    
    bool loadMIDI(const std::string& name, const std::string& filename);
    void playMIDI(const std::string& name, bool loop = false);
    void stopMIDI();
    void setMIDIVolume(float volume);
    
    void playMusic(const std::string& name, bool loop = false);
    void stopMusic();
    void setMusicVolume(float volume);
    
private:
#ifndef NO_FLUIDSYNTH
    fluid_settings_t* settings_;
    fluid_synth_t* synth_;
    fluid_player_t* player_;
    fluid_audio_driver_t* driver_;
#endif
    std::string currentMIDI_;
    
    std::unordered_map<std::string, std::unique_ptr<sf::Music>> musicTracks_;
};