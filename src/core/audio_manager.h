#pragma once

#include <string>
#include <memory>
#include <unordered_map>

#ifdef ENABLE_AUDIO
#include <SDL2/SDL_mixer.h>
#endif

namespace core::audio
{
    class AudioManager
    {
    public:
        AudioManager();
        ~AudioManager();
        
        // Initialize audio system
        bool initialize();
        void shutdown();
        
        // Load audio files
        bool load_music(const std::string& path, const std::string& name);
        bool load_sound(const std::string& path, const std::string& name);
        
        // Play audio
        void play_music(const std::string& name, int loops = -1); // -1 = loop forever
        void play_sound(const std::string& name, int loops = 0);
        
        // Control
        void stop_music();
        void pause_music();
        void resume_music();
        bool is_music_playing() const;
        
        // Volume control (0.0 to 1.0)
        void set_master_volume(float volume);
        void set_music_volume(float volume);
        void set_sound_volume(float volume);
        float get_master_volume() const { return m_master_volume; }
        float get_music_volume() const { return m_music_volume; }
        float get_sound_volume() const { return m_sound_volume; }
        
        // Adjust volume (for +/- keys)
        void increase_volume(float step = 0.1f);
        void decrease_volume(float step = 0.1f);
        
        bool is_available() const { return m_initialized; }
        
    private:
        bool m_initialized = false;
        
        float m_master_volume = 0.5f;  // Start at 50%
        float m_music_volume = 0.7f;
        float m_sound_volume = 0.8f;
        
#ifdef ENABLE_AUDIO
        std::unordered_map<std::string, Mix_Music*> m_music_cache;
        std::unordered_map<std::string, Mix_Chunk*> m_sound_cache;
        std::string m_current_music;
#endif
    };
}




