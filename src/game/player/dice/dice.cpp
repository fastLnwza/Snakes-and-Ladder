#include "dice.h"

#include <glm/gtc/matrix_transform.hpp>
#include <random>
#include <chrono>
#include <cmath>
#include <iostream>

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
        
        // Generate random dice result (1-6)
        int random_dice_result()
        {
            std::uniform_int_distribution<int> dice_distribution(1, 6);
            auto& rng = get_rng();
            return dice_distribution(rng);
        }

        // Generate random rotation velocity for rolling animation
        glm::vec3 random_rotation_velocity()
        {
            std::uniform_real_distribution<float> dist(-15.0f, 15.0f);
            auto& rng = get_rng();
            return glm::vec3(dist(rng), dist(rng), dist(rng));
        }
        
        // Convert dice result (1-6, or 7 for testing) to rotation angles to show the correct face
        // This assumes standard dice orientation where:
        // - Face 1 is opposite face 6
        // - Face 2 is opposite face 5  
        // - Face 3 is opposite face 4
        glm::vec3 result_to_rotation(int result)
        {
            // Map each face to appropriate rotation
            // Adjust these values based on your dice model orientation
            // Fixed: 3 and 6 were swapped
            switch (result)
            {
                case 1: return glm::vec3(-90.0f, 0.0f, 0.0f);    // Rotate X -90 (was 4)
                case 2: return glm::vec3(0.0f, 0.0f, 90.0f);     // Rotate Z 90
                case 3: return glm::vec3(180.0f, 0.0f, 0.0f);    // Rotate X 180 (was 6)
                case 4: return glm::vec3(0.0f, 0.0f, 0.0f);      // Top face (was 1)
                case 5: return glm::vec3(0.0f, 0.0f, -90.0f);    // Rotate Z -90
                case 6: return glm::vec3(90.0f, 0.0f, 0.0f);     // Rotate X 90 (was 3)
                case 7: return glm::vec3(0.0f, 45.0f, 0.0f);     // For testing: default view angle
                default: return glm::vec3(0.0f, 45.0f, 0.0f);    // Default view angle
            }
        }
    }

    void initialize(DiceState& state, const glm::vec3& position)
    {
        state.position = position;
        state.velocity = glm::vec3(0.0f);
        state.target_position = position;
        state.rotation = glm::vec3(0.0f);
        state.rotation_velocity = glm::vec3(0.0f);
        state.roll_duration = 1.0f;
        state.roll_timer = 0.0f;
        state.display_duration = 3.0f;  // 3 seconds to display result
        state.display_timer = 0.0f;
        state.is_rolling = false;
        state.is_falling = false;
        state.is_displaying = false;
        state.result = 0;
        state.pending_result = 0;
        state.scale = 1.0f;
        state.fall_height = 10.0f;
        state.gravity = 20.0f;
        state.bounce_restitution = 0.6f;
        state.ground_y = 0.0f;
    }

    void start_roll(DiceState& state, const glm::vec3& target_position, float fall_height)
    {
        // Don't lock the result yet - we'll randomize it when the dice actually stops
        // This makes the dice truly random, not predetermined
        
        state.is_rolling = true;
        state.is_falling = true;
        state.roll_timer = 0.0f;
        state.result = 0;  // Don't set result yet - wait until bounce stops
        state.pending_result = 0;  // Don't set result yet - randomize when dice stops
        state.target_position = target_position;
        state.fall_height = fall_height;
        
        // Start from above the target position
        state.position = target_position;
        state.position.y += fall_height;
        
        // Start with zero velocity (will fall due to gravity)
        state.velocity = glm::vec3(0.0f, 0.0f, 0.0f);
        
        // Calculate roll duration based on fall time
        // Using physics: h = 0.5 * g * t^2, so t = sqrt(2h/g)
        // Add extra time for visibility
        float fall_time = std::sqrt(2.0f * fall_height / state.gravity);
        state.roll_duration = fall_time + 0.5f; // Add 0.5 seconds after landing for visibility
        
        // Faster rotation velocity for more visible rolling animation
        state.rotation_velocity = random_rotation_velocity() * 2.0f;
    }

    void update(DiceState& state, float delta_time, float board_half_width, float board_half_height)
    {
        if (state.is_rolling)
        {
            state.roll_timer += delta_time;
            
            // Rotate the dice during rolling animation - keep rotating while bouncing
            // Don't lock rotation until dice stops bouncing completely
            if (state.rotation_velocity.x != 0.0f || state.rotation_velocity.y != 0.0f || state.rotation_velocity.z != 0.0f)
            {
                state.rotation += state.rotation_velocity * delta_time;
            }
            
            // Handle falling animation
            if (state.is_falling)
            {
                // Apply gravity
                state.velocity.y -= state.gravity * delta_time;
                
                // Update position based on velocity
                state.position += state.velocity * delta_time;
                
                // Check for collisions with map boundaries and bounce
                float dice_radius = state.scale * 0.5f;
                
                // Bounce off left/right boundaries (X axis)
                if (state.position.x - dice_radius <= -board_half_width)
                {
                    state.position.x = -board_half_width + dice_radius;
                    state.velocity.x *= -state.bounce_restitution;
                    state.velocity.y *= 0.95f; // Slight energy loss on bounce
                    state.velocity.z *= 0.95f;
                }
                else if (state.position.x + dice_radius >= board_half_width)
                {
                    state.position.x = board_half_width - dice_radius;
                    state.velocity.x *= -state.bounce_restitution;
                    state.velocity.y *= 0.95f;
                    state.velocity.z *= 0.95f;
                }
                
                // Bounce off front/back boundaries (Z axis)
                if (state.position.z - dice_radius <= -board_half_height)
                {
                    state.position.z = -board_half_height + dice_radius;
                    state.velocity.z *= -state.bounce_restitution;
                    state.velocity.x *= 0.95f;
                    state.velocity.y *= 0.95f;
                }
                else if (state.position.z + dice_radius >= board_half_height)
                {
                    state.position.z = board_half_height - dice_radius;
                    state.velocity.z *= -state.bounce_restitution;
                    state.velocity.x *= 0.95f;
                    state.velocity.y *= 0.95f;
                }
                
                // Check if dice has reached or passed the ground level
                // Use dice_radius already defined above
                // For a cube dice, the diagonal from center to corner is radius * sqrt(3) ≈ 1.73 * radius
                // Use much larger offset (3x radius or fixed value) to ensure even the corners don't sink into tiles
                // Since dice scale is small (player_radius * 0.167), use fixed offset relative to scale
                float ground_collision_y = state.ground_y + state.scale * 1.5f; // Fixed offset based on scale (ensures dice sits properly)
                if (state.position.y <= ground_collision_y)
                {
                    // Bounce off the ground (with large offset to prevent sinking into tiles, including corners)
                    state.position.y = ground_collision_y;
                    state.velocity.y *= -state.bounce_restitution; // Bounce with energy loss
                    state.velocity.x *= 0.9f; // Slow down horizontal movement
                    state.velocity.z *= 0.9f;
                    
                    // If velocity is very small, stop bouncing and land
                    if (std::abs(state.velocity.y) < 0.5f && std::abs(state.velocity.x) < 0.1f && std::abs(state.velocity.z) < 0.1f)
                    {
                        // Place dice clearly on top of tiles to prevent sinking (including corners)
                        // Use fixed offset based on scale to ensure dice sits properly above tiles
                        state.position.y = state.ground_y + state.scale * 1.5f; // Fixed offset based on scale
                        state.velocity = glm::vec3(0.0f);
                        state.is_falling = false;
                        // Don't stop rotation yet - keep rotating until displaying result
                        // rotation_velocity will continue to rotate the dice
                    }
                }
            }
            
            // Continue rotating while bouncing/falling - don't lock rotation yet
            // Rotation continues through rotation_velocity
        }
        
        // After rolling is done, check if dice has stopped bouncing and ready to show result
        // Only set result and lock rotation when dice has completely stopped bouncing
        // IMPORTANT: Check this AFTER the is_rolling block to catch when dice stops
        // Also check that result hasn't been set yet to prevent overwriting
        if (state.is_rolling && !state.is_falling && !state.is_displaying && state.result == 0)
        {
            // Wait a tiny bit more to ensure dice is completely still
            // Check if velocity is truly zero (dice has stopped bouncing completely)
            if (std::abs(state.velocity.x) < 0.01f && std::abs(state.velocity.y) < 0.01f && std::abs(state.velocity.z) < 0.01f)
            {
                // Dice has completely stopped bouncing - NOW randomize the result
                // This makes the dice truly random, not predetermined
                int final_result = random_dice_result();
                
                // CRITICAL: Verify result is valid (1-7 for testing)
                // Note: 7 is allowed for testing minigame at tile 7
                if (final_result < 1 || final_result > 7)
                {
                    std::cerr << "ERROR: Invalid random result: " << final_result << "! Resetting to 1." << std::endl;
                    final_result = 1;
                }
                
                // Set result IMMEDIATELY
                state.result = final_result;
                
                state.is_rolling = false;
                state.rotation_velocity = glm::vec3(0.0f);  // Stop rotation
                
                // Set rotation to show the correct face based on result
                // IMPORTANT: Use final_result to ensure we use the randomized value
                glm::vec3 result_rotation = result_to_rotation(final_result);
                // Set final rotation to show result face - make sure dice is parallel to ground (flat)
                // For a dice to be flat on ground, we need to rotate to show the face, but keep it level
                state.rotation = glm::vec3(result_rotation.x, result_rotation.y, result_rotation.z);
                // Add a slight viewing angle (optional) but keep dice flat
                // Don't add 45 degrees to Y as it makes dice tilted
                
                // Start displaying result
                state.is_displaying = true;
                state.display_timer = 0.0f;
            }
        }
        
        // Update display timer
        if (state.is_displaying)
        {
            state.display_timer += delta_time;
            
            // Check if display duration has passed
            if (state.display_timer >= state.display_duration)
            {
                state.is_displaying = false;
                // Don't clear result immediately - keep it until new roll starts
                // state.result = 0;  // Clear result - commented to prevent showing 0 during rapid clicks
            }
        }
        
        // Ensure result is cleared when not rolling/displaying
        if (!state.is_rolling && !state.is_falling && !state.is_displaying && state.result == 0)
        {
            // Result is already 0, nothing to do
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

