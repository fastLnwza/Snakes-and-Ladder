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
        state.target_rotation = glm::vec3(0.0f);
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
        // Randomize result IMMEDIATELY when roll starts
        // This allows animation to be consistent with the final result
        int final_result = random_dice_result();
        state.result = final_result;
        state.pending_result = final_result;
        
        // Calculate target rotation for the result
        glm::vec3 target_rotation = result_to_rotation(final_result);
        
        state.is_rolling = true;
        state.is_falling = true;
        state.roll_timer = 0.0f;
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
        
        // Store target rotation for smooth interpolation
        state.target_rotation = target_rotation;
        
        // Start with random rotation but add strong rotation velocity that will help reach target
        // Use random rotation velocity for natural rolling, but with strong bias towards target
        glm::vec3 random_vel = random_rotation_velocity() * 2.0f;
        // Add a strong component that helps reach target rotation
        glm::vec3 rotation_diff = target_rotation - state.rotation;
        
        // Normalize angles to -180 to 180 range for proper bias calculation
        auto normalize_angle = [](float angle) {
            while (angle > 180.0f) angle -= 360.0f;
            while (angle < -180.0f) angle += 360.0f;
            return angle;
        };
        
        rotation_diff.x = normalize_angle(rotation_diff.x);
        rotation_diff.y = normalize_angle(rotation_diff.y);
        rotation_diff.z = normalize_angle(rotation_diff.z);
        
        // Normalize and scale the difference to create a strong bias
        float diff_magnitude = glm::length(rotation_diff);
        if (diff_magnitude > 0.01f)
        {
            // Much stronger bias (20.0f instead of 5.0f) to ensure dice rolls towards target
            glm::vec3 bias = (rotation_diff / diff_magnitude) * 20.0f; // Strong bias strength
            state.rotation_velocity = random_vel + bias;
        }
        else
        {
            state.rotation_velocity = random_vel;
        }
    }

    void update(DiceState& state, float delta_time, float board_half_width, float board_half_height)
    {
        if (state.is_rolling)
        {
            state.roll_timer += delta_time;
            
            // Rotate the dice during rolling animation - keep rotating while bouncing
            // Continuously adjust rotation velocity to bias towards target during rolling
            if (state.rotation_velocity.x != 0.0f || state.rotation_velocity.y != 0.0f || state.rotation_velocity.z != 0.0f)
            {
                state.rotation += state.rotation_velocity * delta_time;
                
                // Continuously apply bias towards target rotation while rolling
                // This ensures dice gradually moves towards the correct face during animation
                if (state.result > 0)
                {
                    glm::vec3 target_rot = result_to_rotation(state.result);
                    glm::vec3 rotation_diff = target_rot - state.rotation;
                    
                    // Normalize angles to -180 to 180 range
                    auto normalize_angle = [](float angle) {
                        while (angle > 180.0f) angle -= 360.0f;
                        while (angle < -180.0f) angle += 360.0f;
                        return angle;
                    };
                    
                    rotation_diff.x = normalize_angle(rotation_diff.x);
                    rotation_diff.y = normalize_angle(rotation_diff.y);
                    rotation_diff.z = normalize_angle(rotation_diff.z);
                    
                    float diff_magnitude = glm::length(rotation_diff);
                    if (diff_magnitude > 1.0f) // Only apply bias if still far from target
                    {
                        // Apply continuous bias during rolling (weaker than initial bias)
                        glm::vec3 bias = (rotation_diff / diff_magnitude) * 10.0f * delta_time;
                        state.rotation_velocity += bias;
                        
                        // Limit rotation velocity to prevent excessive spinning
                        float max_vel = 30.0f;
                        if (glm::length(state.rotation_velocity) > max_vel)
                        {
                            state.rotation_velocity = glm::normalize(state.rotation_velocity) * max_vel;
                        }
                    }
                }
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
        // Snap to target rotation immediately when dice stops bouncing
        // IMPORTANT: Check this AFTER the is_rolling block to catch when dice stops
        if (state.is_rolling && !state.is_falling && !state.is_displaying && state.result > 0)
        {
            // Wait a tiny bit more to ensure dice is completely still
            // Check if velocity is truly zero (dice has stopped bouncing completely)
            if (std::abs(state.velocity.x) < 0.01f && std::abs(state.velocity.y) < 0.01f && std::abs(state.velocity.z) < 0.01f)
            {
                // Dice has completely stopped bouncing - snap to target rotation immediately
                // Use the result that was already randomized at start_roll
                glm::vec3 target_rot = result_to_rotation(state.result);
                
                // Snap to target rotation immediately (no slow interpolation)
                state.rotation = target_rot;
                state.is_rolling = false;
                state.rotation_velocity = glm::vec3(0.0f);  // Stop rotation
                
                // Start displaying result immediately
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

