#include "dice.h"

#include <glm/gtc/matrix_transform.hpp>
#include <random>
#include <chrono>
#include <cmath>

namespace game::player::dice
{
    namespace
    {
        std::mt19937& get_rng()
        {
            static std::mt19937 rng(static_cast<unsigned int>(
                std::chrono::steady_clock::now().time_since_epoch().count()));
            return rng;
        }

        // Generate random rotation velocity for rolling animation
        glm::vec3 random_rotation_velocity()
        {
            std::uniform_real_distribution<float> dist(-10.0f, 10.0f);
            auto& rng = get_rng();
            return glm::vec3(dist(rng), dist(rng), dist(rng));
        }
    }

    void initialize(DiceState& state, const glm::vec3& position)
    {
        state.position = position;
        state.rotation = glm::vec3(0.0f);
        state.rotation_velocity = glm::vec3(0.0f);
        state.roll_duration = 1.0f;
        state.roll_timer = 0.0f;
        state.is_rolling = false;
        state.result = 0;
        state.scale = 1.0f;
    }

    void start_roll(DiceState& state, int result)
    {
        state.is_rolling = true;
        state.roll_timer = 0.0f;
        state.result = result;
        state.rotation_velocity = random_rotation_velocity();
    }

    void update(DiceState& state, float delta_time)
    {
        if (state.is_rolling)
        {
            state.roll_timer += delta_time;
            
            // Rotate the dice during rolling animation
            state.rotation += state.rotation_velocity * delta_time;
            
            // Normalize rotation to keep it in reasonable range (optional)
            // Keep rotations as they are for continuous animation

            // Check if roll is complete
            if (state.roll_timer >= state.roll_duration)
            {
                state.is_rolling = false;
                state.rotation_velocity = glm::vec3(0.0f);
                
                // Set final rotation based on dice result (optional - for visual alignment)
                // For now, just stop the rotation
            }
        }
    }

    glm::mat4 get_transform(const DiceState& state)
    {
        glm::mat4 transform = glm::mat4(1.0f);
        
        // Translate to position
        transform = glm::translate(transform, state.position);
        
        // Rotate (apply rotations in XYZ order)
        transform = glm::rotate(transform, glm::radians(state.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        transform = glm::rotate(transform, glm::radians(state.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        transform = glm::rotate(transform, glm::radians(state.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        
        // Scale
        transform = glm::scale(transform, glm::vec3(state.scale));
        
        return transform;
    }

    bool is_roll_complete(const DiceState& state)
    {
        return !state.is_rolling && state.result > 0;
    }
}

