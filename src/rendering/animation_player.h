#pragma once

#include "gltf_loader.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <map>

// Animation player state
struct AnimationPlayerState
{
    const GLTFAnimation* current_animation = nullptr;
    float animation_time = 0.0f;
    bool is_playing = false;
    bool loop = true;
    float playback_speed = 1.0f;
    
    // Cached transforms per node (by node name)
    std::map<std::string, glm::mat4> node_transforms;
};

// Animation player functions
namespace animation_player
{
    // Start playing an animation
    void play_animation(AnimationPlayerState& state, const GLTFAnimation* animation, bool loop = true, float speed = 1.0f);
    
    // Stop current animation
    void stop_animation(AnimationPlayerState& state);
    
    // Update animation (call every frame with delta_time)
    void update(AnimationPlayerState& state, float delta_time);
    
    // Get transform for a specific node at current animation time
    glm::mat4 get_node_transform(const AnimationPlayerState& state, const std::string& node_name);
    
    // Check if animation is playing
    bool is_playing(const AnimationPlayerState& state);
    
    // Get current animation name
    std::string get_current_animation_name(const AnimationPlayerState& state);
}




