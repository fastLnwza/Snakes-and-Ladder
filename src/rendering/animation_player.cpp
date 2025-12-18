#include "animation_player.h"
#include <algorithm>
#include <cmath>

namespace animation_player
{
    // Helper function to interpolate between two keyframes
    template<typename T>
    T interpolate_keyframes(const std::vector<float>& times, const std::vector<T>& values, float current_time, T (*lerp)(const T&, const T&, float))
    {
        if (times.empty() || values.empty())
            return T{};
        
        if (times.size() == 1)
            return values[0];
        
        // Clamp time to animation duration
        float anim_duration = times.back();
        if (anim_duration > 0.0f)
        {
            current_time = std::fmod(current_time, anim_duration);
        }
        
        // Find the two keyframes to interpolate between
        size_t keyframe_index = 0;
        for (size_t i = 0; i < times.size() - 1; ++i)
        {
            if (current_time >= times[i] && current_time <= times[i + 1])
            {
                keyframe_index = i;
                break;
            }
            if (i == times.size() - 2)
            {
                keyframe_index = i;
            }
        }
        
        // Clamp to valid range
        if (keyframe_index >= times.size() - 1)
            keyframe_index = times.size() - 2;
        
        float t0 = times[keyframe_index];
        float t1 = times[keyframe_index + 1];
        float t = (t1 > t0) ? ((current_time - t0) / (t1 - t0)) : 0.0f;
        t = std::clamp(t, 0.0f, 1.0f);
        
        const T& v0 = values[keyframe_index];
        const T& v1 = values[keyframe_index + 1];
        
        return lerp(v0, v1, t);
    }
    
    // Lerp functions for different types
    glm::vec3 lerp_vec3(const glm::vec3& a, const glm::vec3& b, float t)
    {
        return glm::mix(a, b, t);
    }
    
    glm::quat lerp_quat(const glm::quat& a, const glm::quat& b, float t)
    {
        return glm::slerp(a, b, t); // Spherical interpolation for rotations
    }
    
    void play_animation(AnimationPlayerState& state, const GLTFAnimation* animation, bool loop, float speed)
    {
        if (!animation)
            return;
        
        state.current_animation = animation;
        state.animation_time = 0.0f;
        state.is_playing = true;
        state.loop = loop;
        state.playback_speed = speed;
        state.node_transforms.clear();
    }
    
    void stop_animation(AnimationPlayerState& state)
    {
        state.is_playing = false;
        state.current_animation = nullptr;
        state.animation_time = 0.0f;
        state.node_transforms.clear();
    }
    
    void update(AnimationPlayerState& state, float delta_time)
    {
        if (!state.is_playing || !state.current_animation)
            return;
        
        // Update animation time
        state.animation_time += delta_time * state.playback_speed;
        
        // Handle looping
        if (state.loop && state.current_animation->duration > 0.0f)
        {
            state.animation_time = std::fmod(state.animation_time, state.current_animation->duration);
        }
        else if (state.animation_time >= state.current_animation->duration)
        {
            state.animation_time = state.current_animation->duration;
            state.is_playing = false; // Stop at end if not looping
        }
        
        // Update transforms for all nodes in the animation
        state.node_transforms.clear();
        
        for (const auto& channel : state.current_animation->channels)
        {
            glm::mat4 transform = glm::mat4(1.0f);
            
            // Get current transform values from keyframes
            if (channel.target_path == "translation" && !channel.translation_keys.empty())
            {
                glm::vec3 translation = interpolate_keyframes(
                    channel.keyframe_times,
                    channel.translation_keys,
                    state.animation_time,
                    lerp_vec3
                );
                transform = glm::translate(transform, translation);
            }
            else if (channel.target_path == "rotation" && !channel.rotation_keys.empty())
            {
                glm::quat rotation = interpolate_keyframes(
                    channel.keyframe_times,
                    channel.rotation_keys,
                    state.animation_time,
                    lerp_quat
                );
                transform = glm::mat4_cast(rotation);
            }
            else if (channel.target_path == "scale" && !channel.scale_keys.empty())
            {
                glm::vec3 scale = interpolate_keyframes(
                    channel.keyframe_times,
                    channel.scale_keys,
                    state.animation_time,
                    lerp_vec3
                );
                transform = glm::scale(transform, scale);
            }
            
            // Store transform for this node (by name)
            // Note: For now we store by node name. In a full implementation,
            // you'd want to map nodes properly and apply transforms hierarchically
            state.node_transforms[channel.target_node_name] = transform;
        }
    }
    
    glm::mat4 get_node_transform(const AnimationPlayerState& state, const std::string& node_name)
    {
        auto it = state.node_transforms.find(node_name);
        if (it != state.node_transforms.end())
        {
            return it->second;
        }
        return glm::mat4(1.0f); // Identity if not found
    }
    
    bool is_playing(const AnimationPlayerState& state)
    {
        return state.is_playing && state.current_animation != nullptr;
    }
    
    std::string get_current_animation_name(const AnimationPlayerState& state)
    {
        if (state.current_animation)
            return state.current_animation->name;
        return "";
    }
}




