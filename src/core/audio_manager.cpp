#include "audio_manager.h"
#include <iostream>
#include <algorithm>

#ifdef ENABLE_AUDIO
#include <SDL2/SDL_mixer.h>
#endif

namespace core::audio
{
    AudioManager::AudioManager()
    {
    }
    
    AudioManager::~AudioManager()
    {
        shutdown();
    }
    
    bool AudioManager::initialize()
    {
#ifdef ENABLE_AUDIO
        if (m_initialized)
            return true;
        
        // Initialize SDL_mixer
        if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
        {
            std::cerr << "Failed to initialize SDL_mixer: " << Mix_GetError() << std::endl;
            return false;
        }
        
        // Allocate channels for sound effects
        Mix_AllocateChannels(16);
        
        // Apply initial volume settings
        set_master_volume(m_master_volume);
        set_music_volume(m_music_volume);
        
        m_initialized = true;
        std::cout << "Audio system initialized successfully (Volume: " << (int)(m_master_volume * 100) << "%)" << std::endl;
        return true;
#else
        std::cout << "Audio disabled (SDL2_mixer not found)" << std::endl;
        return false;
#endif
    }
    
    void AudioManager::shutdown()
    {
#ifdef ENABLE_AUDIO
        if (!m_initialized)
            return;
        
        stop_music();
        
        // Free music
        for (auto& [name, music] : m_music_cache)
        {
            if (music)
                Mix_FreeMusic(music);
        }
        m_music_cache.clear();
        
        // Free sounds
        for (auto& [name, chunk] : m_sound_cache)
        {
            if (chunk)
                Mix_FreeChunk(chunk);
        }
        m_sound_cache.clear();
        
        Mix_CloseAudio();
        m_initialized = false;
        std::cout << "Audio system shut down" << std::endl;
#endif
    }
    
    bool AudioManager::load_music(const std::string& path, const std::string& name)
    {
#ifdef ENABLE_AUDIO
        if (!m_initialized)
        {
            std::cerr << "Audio system not initialized" << std::endl;
            return false;
        }
        
        // Check if already loaded
        if (m_music_cache.find(name) != m_music_cache.end())
        {
            std::cout << "Music '" << name << "' already loaded" << std::endl;
            return true;
        }
        
        Mix_Music* music = Mix_LoadMUS(path.c_str());
        if (!music)
        {
            std::cerr << "Failed to load music '" << path << "': " << Mix_GetError() << std::endl;
            return false;
        }
        
        m_music_cache[name] = music;
        std::cout << "Loaded music: " << name << " from " << path << std::endl;
        return true;
#else
        (void)path;
        (void)name;
        return false;
#endif
    }
    
    bool AudioManager::load_sound(const std::string& path, const std::string& name)
    {
#ifdef ENABLE_AUDIO
        if (!m_initialized)
        {
            std::cerr << "Audio system not initialized" << std::endl;
            return false;
        }
        
        // Check if already loaded
        if (m_sound_cache.find(name) != m_sound_cache.end())
        {
            std::cout << "Sound '" << name << "' already loaded" << std::endl;
            return true;
        }
        
        Mix_Chunk* chunk = Mix_LoadWAV(path.c_str());
        if (!chunk)
        {
            std::cerr << "Failed to load sound '" << path << "': " << Mix_GetError() << std::endl;
            return false;
        }
        
        m_sound_cache[name] = chunk;
        std::cout << "Loaded sound: " << name << " from " << path << std::endl;
        return true;
#else
        (void)path;
        (void)name;
        return false;
#endif
    }
    
    void AudioManager::play_music(const std::string& name, int loops)
    {
#ifdef ENABLE_AUDIO
        if (!m_initialized)
            return;
        
        auto it = m_music_cache.find(name);
        if (it == m_music_cache.end())
        {
            std::cerr << "Music '" << name << "' not loaded" << std::endl;
            return;
        }
        
        if (Mix_PlayMusic(it->second, loops) == -1)
        {
            std::cerr << "Failed to play music: " << Mix_GetError() << std::endl;
            return;
        }
        
        m_current_music = name;
        set_music_volume(m_music_volume); // Apply current volume
        std::cout << "Playing music: " << name << std::endl;
#else
        (void)name;
        (void)loops;
#endif
    }
    
    void AudioManager::play_sound(const std::string& name, int loops)
    {
#ifdef ENABLE_AUDIO
        if (!m_initialized)
            return;
        
        auto it = m_sound_cache.find(name);
        if (it == m_sound_cache.end())
        {
            std::cerr << "Sound '" << name << "' not loaded" << std::endl;
            return;
        }
        
        int channel = Mix_PlayChannel(-1, it->second, loops);
        if (channel == -1)
        {
            std::cerr << "Failed to play sound: " << Mix_GetError() << std::endl;
            return;
        }
        
        // Apply volume to this channel
        int volume = static_cast<int>(m_sound_volume * m_master_volume * MIX_MAX_VOLUME);
        Mix_Volume(channel, volume);
#else
        (void)name;
        (void)loops;
#endif
    }
    
    void AudioManager::stop_music()
    {
#ifdef ENABLE_AUDIO
        if (m_initialized)
            Mix_HaltMusic();
        m_current_music.clear();
#endif
    }
    
    void AudioManager::pause_music()
    {
#ifdef ENABLE_AUDIO
        if (m_initialized)
            Mix_PauseMusic();
#endif
    }
    
    void AudioManager::resume_music()
    {
#ifdef ENABLE_AUDIO
        if (m_initialized)
            Mix_ResumeMusic();
#endif
    }
    
    bool AudioManager::is_music_playing() const
    {
#ifdef ENABLE_AUDIO
        return m_initialized && Mix_PlayingMusic() != 0;
#else
        return false;
#endif
    }
    
    void AudioManager::set_master_volume(float volume)
    {
        m_master_volume = std::clamp(volume, 0.0f, 1.0f);
        
#ifdef ENABLE_AUDIO
        if (m_initialized)
        {
            // Update music volume
            int music_vol = static_cast<int>(m_music_volume * m_master_volume * MIX_MAX_VOLUME);
            Mix_VolumeMusic(music_vol);
        }
#endif
    }
    
    void AudioManager::set_music_volume(float volume)
    {
        m_music_volume = std::clamp(volume, 0.0f, 1.0f);
        
#ifdef ENABLE_AUDIO
        if (m_initialized)
        {
            int vol = static_cast<int>(m_music_volume * m_master_volume * MIX_MAX_VOLUME);
            Mix_VolumeMusic(vol);
        }
#endif
    }
    
    void AudioManager::set_sound_volume(float volume)
    {
        m_sound_volume = std::clamp(volume, 0.0f, 1.0f);
        // Sound volume is applied per-channel when playing
    }
    
    void AudioManager::increase_volume(float step)
    {
        float new_volume = m_master_volume + step;
        set_master_volume(new_volume);
    }
    
    void AudioManager::decrease_volume(float step)
    {
        float new_volume = m_master_volume - step;
        set_master_volume(new_volume);
    }
}




