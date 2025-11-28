#pragma once

#include <glm/glm.hpp>

namespace game::player
{
    struct PlayerState
    {
        glm::vec3 position{};
        int current_tile_index = 0;
        bool is_stepping = false;
        glm::vec3 step_start_position{};
        glm::vec3 step_end_position{};
        float step_timer = 0.0f;
        int steps_remaining = 0;
        int last_dice_result = 0;
        bool previous_space_state = false;
        
        float ground_y = 0.0f;
        float radius = 0.4f;
        float step_duration = 0.55f;
    };

    void initialize(PlayerState& state, const glm::vec3& start_position, float ground_y, float radius);
    void update(PlayerState& state, float delta_time, bool space_just_pressed, int final_tile_index, bool can_start_walking = true);
    void roll_dice(PlayerState& state);  // Kept for compatibility, does nothing now
    void set_dice_result(PlayerState& state, int result);  // Set result after dice finishes rolling
    void warp_to_tile(PlayerState& state, int tile_index);
    glm::vec3 get_position(const PlayerState& state);
    int get_current_tile(const PlayerState& state);
}

