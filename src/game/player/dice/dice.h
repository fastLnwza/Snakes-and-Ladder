#pragma once

#include <glm/glm.hpp>

namespace game::player::dice
{
    struct DiceState
    {
        glm::vec3 position{};
        glm::vec3 velocity{};  // Velocity for falling animation
        glm::vec3 target_position{};  // Target position above player's head
        glm::vec3 rotation{};
        glm::vec3 rotation_velocity{};
        glm::vec3 target_rotation{};  // Target rotation to show the correct face
        float roll_duration = 1.0f;
        float roll_timer = 0.0f;
        float display_duration = 3.0f;  // Time to display result after landing (3 seconds)
        float display_timer = 0.0f;  // Timer for displaying result
        bool is_rolling = false;
        bool is_falling = false;
        bool is_displaying = false;  // Whether dice is displaying result
        int result = 0;  // Final result to display (set immediately when roll starts)
        int pending_result = 0;  // Result from player roll (stored but not shown until bounce stops)
        float scale = 1.0f;
        float fall_height = 10.0f;  // Height to start falling from
        float gravity = 20.0f;  // Gravity acceleration
        float bounce_restitution = 0.6f;  // Bounce coefficient (0.0 = no bounce, 1.0 = perfect bounce)
        float ground_y = 0.0f;  // Ground level Y coordinate
    };

    void initialize(DiceState& state, const glm::vec3& position);
    void start_roll(DiceState& state, const glm::vec3& target_position, float fall_height = 10.0f);  // Dice rolls its own number
    void update(DiceState& state, float delta_time, float board_half_width, float board_half_height);
    glm::mat4 get_transform(const DiceState& state);
    bool is_roll_complete(const DiceState& state);
    int get_result(const DiceState& state);  // Get the rolled result after dice stops
}


