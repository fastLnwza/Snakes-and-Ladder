#include "player.h"

#include "../../game/map/board.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <glm/gtc/constants.hpp>
#include <random>

namespace game::player
{
    using namespace game::map;

    namespace
    {
        std::mt19937& get_rng()
        {
            static std::mt19937 rng(static_cast<unsigned int>(
                std::chrono::steady_clock::now().time_since_epoch().count()));
            return rng;
        }

        std::uniform_int_distribution<int>& get_dice_distribution()
        {
            static std::uniform_int_distribution<int> dice_distribution(1, 6);
            return dice_distribution;
        }

        void schedule_step(PlayerState& state, int final_tile_index)
        {
            // Only check if steps remaining and not already stepping
            // Don't check final_tile_index here - let player walk the full dice result
            if (state.steps_remaining <= 0 || state.is_stepping)
            {
                return;
            }

            state.step_start_position = tile_center_world(state.current_tile_index);
            state.step_start_position.y = state.ground_y;

            state.step_end_position = tile_center_world(state.current_tile_index + 1);
            state.step_end_position.y = state.ground_y;

            state.is_stepping = true;
            state.step_timer = 0.0f;
        }
    }

    void initialize(PlayerState& state, const glm::vec3& start_position, float ground_y, float radius)
    {
        state.position = start_position;
        state.position.y = ground_y;
        state.current_tile_index = 0;
        state.is_stepping = false;
        state.step_start_position = state.position;
        state.step_end_position = state.position;
        state.step_timer = 0.0f;
        state.steps_remaining = 0;
        state.last_dice_result = 0;
        state.previous_space_state = false;
        state.ground_y = ground_y;
        state.radius = radius;
    }

    void roll_dice(PlayerState& state)
    {
        // Don't roll here - dice will roll its own number
        // This function is kept for compatibility but does nothing
        // The actual dice roll happens in dice::start_roll()
        (void)state;  // Suppress unused parameter warning
    }
    
    void set_dice_result(PlayerState& state, int result)
    {
        // Set the dice result after dice has finished rolling
        state.last_dice_result = result;
        state.steps_remaining = result;
    }

    void warp_to_tile(PlayerState& state, int tile_index)
    {
        const int final_tile_index = BOARD_COLUMNS * BOARD_ROWS - 1;
        const int clamped_tile = std::clamp(tile_index, 0, final_tile_index);

        state.current_tile_index = clamped_tile;
        state.steps_remaining = 0;
        state.last_dice_result = 0;
        state.is_stepping = false;
        state.step_timer = 0.0f;

        glm::vec3 tile_position = tile_center_world(clamped_tile);
        tile_position.y = state.ground_y;

        state.position = tile_position;
        state.step_start_position = tile_position;
        state.step_end_position = tile_position;
    }

    void update(PlayerState& state, float delta_time, bool space_just_pressed, int final_tile_index, bool can_start_walking)
    {
        // Don't roll dice here - dice will be rolled when space is pressed in main.cpp
        // The dice will roll its own number, and then we'll use that result
        (void)space_just_pressed;  // Suppress unused parameter warning
        
        // Continue stepping if already started (check this first)
        if (state.is_stepping)
        {
            state.step_timer += delta_time;
            const float t = glm::clamp(state.step_timer / state.step_duration, 0.0f, 1.0f);
            const float ease = t * t * (3.0f - 2.0f * t); // smoothstep
            glm::vec3 interpolated = glm::mix(state.step_start_position, state.step_end_position, ease);
            interpolated.y += std::sin(ease * glm::pi<float>()) * state.radius * 0.18f;
            state.position = interpolated;

            if (t >= 1.0f - 1e-4f)
            {
                state.position = state.step_end_position;
                state.position.y = state.ground_y;
                ++state.current_tile_index;
                --state.steps_remaining;
                state.is_stepping = false;
                // Continue walking if there are steps remaining - use dice result directly
                if (state.steps_remaining > 0)
                {
                    schedule_step(state, final_tile_index);
                }
            }
        }
        else
        {
            state.position.y = state.ground_y;
            
            // Start walking if we have steps remaining and can start
            // This is a safeguard to ensure walking starts even if initial check missed it
            if (can_start_walking && state.steps_remaining > 0 && !state.is_stepping)
            {
                schedule_step(state, final_tile_index);
            }
        }
    }

    glm::vec3 get_position(const PlayerState& state)
    {
        return state.position;
    }

    int get_current_tile(const PlayerState& state)
    {
        return state.current_tile_index;
    }
}

