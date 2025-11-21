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
            if (state.steps_remaining <= 0 || state.is_stepping || state.current_tile_index >= final_tile_index)
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
        state.last_dice_result = get_dice_distribution()(get_rng());
        state.steps_remaining = state.last_dice_result;
    }

    void update(PlayerState& state, float delta_time, bool space_just_pressed, int final_tile_index)
    {
        if (space_just_pressed && !state.is_stepping && state.current_tile_index < final_tile_index)
        {
            roll_dice(state);
            state.steps_remaining = std::min(state.steps_remaining, final_tile_index - state.current_tile_index);
            if (state.steps_remaining > 0)
            {
                schedule_step(state, final_tile_index);
            }
        }

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
                if (state.steps_remaining > 0 && state.current_tile_index < final_tile_index)
                {
                    schedule_step(state, final_tile_index);
                }
            }
        }
        else
        {
            state.position.y = state.ground_y;
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

