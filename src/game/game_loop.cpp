#include "game_loop.h"

#include "../core/window.h"
#include "../game/map/map_manager.h"
#include "../game/player/player.h"
#include "../game/player/dice/dice.h"
#include "../game/minigame/qte_minigame.h"
#include "../game/minigame/tile_memory_minigame.h"
#include "../game/minigame/reaction_minigame.h"
#include "../game/minigame/math_minigame.h"
#include "../game/minigame/pattern_minigame.h"

#include <GLFW/glfw3.h>
#include <algorithm>

namespace game
{
    GameLoop::GameLoop(core::Window& window, core::Camera& camera, GameState& game_state, RenderState& render_state)
        : m_window(window)
        , m_camera(camera)
        , m_game_state(game_state)
        , m_render_state(render_state)
    {
    }

    void GameLoop::update(float delta_time)
    {
        handle_input(delta_time);
        update_minigames(delta_time);
        handle_minigame_results(delta_time);
        update_game_logic(delta_time);
    }

    void GameLoop::handle_input(float delta_time)
    {
        using namespace game::player;
        using namespace game::player::dice;
        using namespace game::map;

        const bool space_pressed = m_window.is_key_pressed(GLFW_KEY_SPACE);
        const bool space_just_pressed = space_pressed && !m_game_state.player_state.previous_space_state;
        
        const bool precision_running = game::minigame::is_running(m_game_state.minigame_state);
        const bool tile_memory_running = game::minigame::tile_memory::is_running(m_game_state.tile_memory_state);
        const bool tile_memory_active = game::minigame::tile_memory::is_active(m_game_state.tile_memory_state);
        const bool reaction_running = game::minigame::is_running(m_game_state.reaction_state);
        const bool math_running = game::minigame::is_running(m_game_state.math_state);
        const bool pattern_running = game::minigame::is_running(m_game_state.pattern_state);
        const bool minigame_running = precision_running || tile_memory_running || reaction_running || math_running || pattern_running;

        // Start dice roll
        if (space_just_pressed && !m_game_state.player_state.is_stepping && 
            m_game_state.player_state.steps_remaining == 0 && 
            !m_game_state.dice_state.is_rolling && 
            !m_game_state.dice_state.is_falling && 
            !minigame_running)
        {
            m_game_state.dice_state.is_displaying = false;
            m_game_state.dice_state.is_rolling = false;
            m_game_state.dice_state.is_falling = false;
            m_game_state.dice_state.display_timer = 0.0f;
            m_game_state.dice_state.roll_timer = 0.0f;
            m_game_state.dice_state.result = 0;
            m_game_state.dice_state.pending_result = 0;

            glm::vec3 player_pos = get_position(m_game_state.player_state);
            glm::vec3 target_pos = player_pos;
            target_pos.y = m_game_state.player_ground_y;
            m_game_state.dice_state.ground_y = m_game_state.player_ground_y;

            const float fall_height = m_game_state.map_length * 0.4f;
            start_roll(m_game_state.dice_state, target_pos, fall_height);
        }

        // Handle precision timing input
        if (precision_running)
        {
            const bool space_down = m_window.is_key_pressed(GLFW_KEY_SPACE);
            const bool space_just_pressed_for_minigame = space_down && !m_game_state.precision_space_was_down;
            m_game_state.precision_space_was_down = space_down;

            if (space_just_pressed_for_minigame)
            {
                game::minigame::stop_timing(m_game_state.minigame_state);
            }
            else if (game::minigame::has_expired(m_game_state.minigame_state))
            {
                game::minigame::stop_timing(m_game_state.minigame_state);
            }
        }
        else
        {
            m_game_state.precision_space_was_down = false;
        }

        // Handle tile memory input
        if (tile_memory_running)
        {
            for (int key_index = 0; key_index < 9; ++key_index)
            {
                const int glfw_key = GLFW_KEY_1 + key_index;
                const bool key_down = m_window.is_key_pressed(glfw_key);
                const bool key_just_pressed = key_down && !m_game_state.tile_memory_previous_keys[key_index];
                if (key_just_pressed)
                {
                    game::minigame::tile_memory::submit_choice(m_game_state.tile_memory_state, key_index + 1);
                }
                m_game_state.tile_memory_previous_keys[key_index] = key_down;
            }
        }
        else
        {
            m_game_state.tile_memory_previous_keys.fill(false);
        }

        // Handle reaction game input
        if (reaction_running)
        {
            const bool space_down = m_window.is_key_pressed(GLFW_KEY_SPACE);
            const bool space_just_pressed_reaction = space_down && !m_game_state.precision_space_was_down;
            if (space_just_pressed_reaction)
            {
                game::minigame::press_key(m_game_state.reaction_state);
            }
        }

        // Handle math game input
        if (math_running)
        {
            for (int key_index = 0; key_index < 9; ++key_index)
            {
                const int glfw_key = GLFW_KEY_1 + key_index;
                const bool key_down = m_window.is_key_pressed(glfw_key);
                const bool key_just_pressed = key_down && !m_game_state.tile_memory_previous_keys[key_index];
                if (key_just_pressed)
                {
                    game::minigame::submit_answer(m_game_state.math_state, key_index + 1);
                }
                m_game_state.tile_memory_previous_keys[key_index] = key_down;
            }
        }

        // Handle pattern game input
        if (pattern_running)
        {
            const bool w_down = m_window.is_key_pressed(GLFW_KEY_W);
            const bool s_down = m_window.is_key_pressed(GLFW_KEY_S);
            const bool a_down = m_window.is_key_pressed(GLFW_KEY_A);
            const bool d_down = m_window.is_key_pressed(GLFW_KEY_D);

            if (w_down && !m_game_state.pattern_previous_keys[0])
            {
                game::minigame::submit_input(m_game_state.pattern_state, 1);
            }
            if (s_down && !m_game_state.pattern_previous_keys[1])
            {
                game::minigame::submit_input(m_game_state.pattern_state, 2);
            }
            if (a_down && !m_game_state.pattern_previous_keys[2])
            {
                game::minigame::submit_input(m_game_state.pattern_state, 3);
            }
            if (d_down && !m_game_state.pattern_previous_keys[3])
            {
                game::minigame::submit_input(m_game_state.pattern_state, 4);
            }

            m_game_state.pattern_previous_keys[0] = w_down;
            m_game_state.pattern_previous_keys[1] = s_down;
            m_game_state.pattern_previous_keys[2] = a_down;
            m_game_state.pattern_previous_keys[3] = d_down;
        }

        // Handle debug warp input
        const bool t_key_down = m_window.is_key_pressed(GLFW_KEY_T);
        if (t_key_down && !m_game_state.debug_warp_state.prev_toggle && !minigame_running)
        {
            m_game_state.debug_warp_state.active = !m_game_state.debug_warp_state.active;
            if (!m_game_state.debug_warp_state.active)
            {
                m_game_state.debug_warp_state.buffer.clear();
                m_game_state.debug_warp_state.digit_previous.fill(false);
            }
        }
        m_game_state.debug_warp_state.prev_toggle = t_key_down;

        if (m_game_state.debug_warp_state.active && minigame_running)
        {
            m_game_state.debug_warp_state.active = false;
            m_game_state.debug_warp_state.buffer.clear();
            m_game_state.debug_warp_state.digit_previous.fill(false);
        }

        if (m_game_state.debug_warp_state.active)
        {
            for (int digit = 0; digit <= 9; ++digit)
            {
                const int key = GLFW_KEY_0 + digit;
                const bool key_down = m_window.is_key_pressed(key);
                const bool key_just_pressed = key_down && !m_game_state.debug_warp_state.digit_previous[digit];
                if (key_just_pressed && m_game_state.debug_warp_state.buffer.size() < 3)
                {
                    m_game_state.debug_warp_state.buffer.push_back(static_cast<char>('0' + digit));
                }
                m_game_state.debug_warp_state.digit_previous[digit] = key_down;
            }

            const bool backspace_down = m_window.is_key_pressed(GLFW_KEY_BACKSPACE);
            if (backspace_down && !m_game_state.debug_warp_state.prev_backspace && !m_game_state.debug_warp_state.buffer.empty())
            {
                m_game_state.debug_warp_state.buffer.pop_back();
            }
            m_game_state.debug_warp_state.prev_backspace = backspace_down;

            const bool enter_down = m_window.is_key_pressed(GLFW_KEY_ENTER) ||
                                    m_window.is_key_pressed(GLFW_KEY_KP_ENTER);
            if (enter_down && !m_game_state.debug_warp_state.prev_enter && !m_game_state.debug_warp_state.buffer.empty())
            {
                try
                {
                    int requested_tile = std::stoi(m_game_state.debug_warp_state.buffer);
                    int zero_based_tile = (requested_tile <= 0) ? 0 : requested_tile - 1;
                    int target_tile = std::clamp(zero_based_tile, 0, m_game_state.final_tile_index);
                    warp_to_tile(m_game_state.player_state, target_tile);
                    m_game_state.last_processed_tile = -1;
                    m_game_state.player_state.previous_space_state = false;

                    m_game_state.dice_state.is_rolling = false;
                    m_game_state.dice_state.is_falling = false;
                    m_game_state.dice_state.is_displaying = false;
                    m_game_state.dice_state.result = 0;
                    m_game_state.dice_state.pending_result = 0;
                    m_game_state.dice_state.roll_timer = 0.0f;
                    m_game_state.dice_state.velocity = glm::vec3(0.0f);
                    m_game_state.dice_state.rotation_velocity = glm::vec3(0.0f);
                    m_game_state.dice_state.position = tile_center_world(target_tile) + glm::vec3(0.0f, m_game_state.player_ground_y + 3.0f, 0.0f);
                    m_game_state.dice_state.target_position = m_game_state.dice_state.position;
                    m_game_state.dice_display_timer = 0.0f;

                    game::minigame::reset(m_game_state.minigame_state);
                    game::minigame::tile_memory::reset(m_game_state.tile_memory_state);
                    game::minigame::reset(m_game_state.reaction_state);
                    game::minigame::reset(m_game_state.math_state);
                    game::minigame::reset(m_game_state.pattern_state);
                    m_game_state.precision_result_applied = false;
                    m_game_state.tile_memory_result_applied = false;
                    m_game_state.reaction_result_applied = false;
                    m_game_state.math_result_applied = false;
                    m_game_state.pattern_result_applied = false;
                    m_game_state.precision_result_display_timer = 3.0f;
                    m_game_state.tile_memory_previous_keys.fill(false);
                    m_game_state.minigame_message.clear();
                    m_game_state.minigame_message_timer = 0.0f;

                    const int display_tile = requested_tile;
                    m_game_state.debug_warp_state.notification = "wrap to " + std::to_string(display_tile) + " [enter]";
                    m_game_state.debug_warp_state.notification_timer = 3.0f;
                    m_game_state.debug_warp_state.buffer.clear();
                    m_game_state.debug_warp_state.active = false;
                    m_game_state.debug_warp_state.digit_previous.fill(false);
                }
                catch (const std::exception&)
                {
                    m_game_state.debug_warp_state.notification = "Invalid tile";
                    m_game_state.debug_warp_state.notification_timer = 3.0f;
                    m_game_state.debug_warp_state.buffer.clear();
                    m_game_state.debug_warp_state.active = false;
                    m_game_state.debug_warp_state.digit_previous.fill(false);
                }
            }
            m_game_state.debug_warp_state.prev_enter = enter_down;
        }
        else
        {
            m_game_state.debug_warp_state.digit_previous.fill(false);
            m_game_state.debug_warp_state.prev_backspace = false;
            m_game_state.debug_warp_state.prev_enter = false;
        }

        if (m_game_state.debug_warp_state.notification_timer > 0.0f)
        {
            m_game_state.debug_warp_state.notification_timer = std::max(0.0f, m_game_state.debug_warp_state.notification_timer - delta_time);
        }

        m_game_state.player_state.previous_space_state = space_pressed;
    }

    void GameLoop::update_game_logic(float delta_time)
    {
        using namespace game::player;
        using namespace game::player::dice;
        using namespace game::map;

        // Check if dice has finished rolling
        const bool dice_ready = !m_game_state.dice_state.is_falling && 
                               !m_game_state.dice_state.is_rolling && 
                               m_game_state.dice_state.result > 0;

        // Transfer dice result to player
        if (dice_ready && m_game_state.player_state.last_dice_result != m_game_state.dice_state.result)
        {
            set_dice_result(m_game_state.player_state, m_game_state.dice_state.result);
            m_game_state.dice_display_timer = 10.0f;
        }

        // Update dice display timer
        if (m_game_state.dice_display_timer > 0.0f)
        {
            m_game_state.dice_display_timer = std::max(0.0f, m_game_state.dice_display_timer - delta_time);
        }

        // Safety check for dice result
        if (dice_ready && m_game_state.player_state.steps_remaining == 0 && m_game_state.dice_state.result > 0)
        {
            if (m_game_state.player_state.last_dice_result != m_game_state.dice_state.result)
            {
                set_dice_result(m_game_state.player_state, m_game_state.dice_state.result);
                m_game_state.dice_display_timer = 10.0f;
            }
        }

        // Update player movement
        const bool precision_running = game::minigame::is_running(m_game_state.minigame_state);
        const bool tile_memory_running = game::minigame::tile_memory::is_running(m_game_state.tile_memory_state);
        const bool tile_memory_active = game::minigame::tile_memory::is_active(m_game_state.tile_memory_state);
        const bool reaction_running = game::minigame::is_running(m_game_state.reaction_state);
        const bool math_running = game::minigame::is_running(m_game_state.math_state);
        const bool pattern_running = game::minigame::is_running(m_game_state.pattern_state);
        const bool minigame_running = precision_running || tile_memory_running || reaction_running || math_running || pattern_running;

        // Check minigame results to get force_walk flag
        bool minigame_force_walk = false;
        if ((game::minigame::is_success(m_game_state.minigame_state) && m_game_state.precision_result_applied) ||
            (game::minigame::tile_memory::is_success(m_game_state.tile_memory_state) && m_game_state.tile_memory_result_applied) ||
            (game::minigame::is_success(m_game_state.reaction_state) && m_game_state.reaction_result_applied) ||
            (game::minigame::is_success(m_game_state.math_state) && m_game_state.math_result_applied) ||
            (game::minigame::is_success(m_game_state.pattern_state) && m_game_state.pattern_result_applied))
        {
            minigame_force_walk = true;
        }

        const bool dice_finished = dice_ready && !minigame_running;
        const bool has_steps_to_walk = m_game_state.player_state.steps_remaining > 0;
        const bool can_walk_now = (dice_finished && has_steps_to_walk) || minigame_force_walk;

        game::player::update(m_game_state.player_state, delta_time, false, m_game_state.final_tile_index, can_walk_now);

        // Final safeguard for player movement
        if (dice_ready && m_game_state.player_state.steps_remaining > 0 && 
            !m_game_state.player_state.is_stepping && 
            !minigame_running &&
            !minigame_force_walk)
        {
            game::player::update(m_game_state.player_state, delta_time, false, m_game_state.final_tile_index, true);
        }

        // Update dice animation
        const float half_width = m_game_state.map_data.board_width * 0.5f;
        const float half_height = m_game_state.map_data.board_height * 0.5f;
        game::player::dice::update(m_game_state.dice_state, delta_time, half_width, half_height);

        // Update minigame message timer
        if (m_game_state.minigame_message_timer > 0.0f)
        {
            m_game_state.minigame_message_timer = std::max(0.0f, m_game_state.minigame_message_timer - delta_time);
        }

        // Check tile activity
        const int current_tile = get_current_tile(m_game_state.player_state);
        if (current_tile != m_game_state.last_processed_tile)
        {
            if (!m_game_state.player_state.is_stepping && m_game_state.player_state.steps_remaining == 0)
            {
                m_game_state.last_processed_tile = current_tile;

                game::map::check_and_apply_ladder(m_game_state.player_state, current_tile, m_game_state.last_processed_tile);

                const bool tile_memory_active_check = game::minigame::tile_memory::is_active(m_game_state.tile_memory_state);
                game::map::check_tile_activity(current_tile,
                                              m_game_state.last_processed_tile,
                                              minigame_running,
                                              tile_memory_active_check,
                                              m_game_state.player_state,
                                              m_game_state.minigame_state,
                                              m_game_state.tile_memory_state,
                                              m_game_state.reaction_state,
                                              m_game_state.math_state,
                                              m_game_state.pattern_state,
                                              m_game_state.minigame_message,
                                              m_game_state.minigame_message_timer,
                                              m_game_state.tile_memory_previous_keys,
                                              m_game_state.precision_space_was_down);
            }
            else
            {
                m_game_state.last_processed_tile = current_tile;
            }
        }
    }

    void GameLoop::update_minigames(float delta_time)
    {
        const bool precision_running = game::minigame::is_running(m_game_state.minigame_state);
        const bool precision_should_advance =
            precision_running || m_game_state.minigame_state.is_showing_time ||
            game::minigame::is_success(m_game_state.minigame_state) || 
            game::minigame::is_failure(m_game_state.minigame_state);
        if (precision_should_advance)
        {
            game::minigame::advance(m_game_state.minigame_state, delta_time);
        }

        if (game::minigame::tile_memory::is_active(m_game_state.tile_memory_state))
        {
            game::minigame::tile_memory::advance(m_game_state.tile_memory_state, delta_time);
        }

        if (game::minigame::is_running(m_game_state.reaction_state))
        {
            game::minigame::advance(m_game_state.reaction_state, delta_time);
        }

        if (game::minigame::is_running(m_game_state.math_state))
        {
            game::minigame::advance(m_game_state.math_state, delta_time);
        }

        if (game::minigame::is_running(m_game_state.pattern_state))
        {
            game::minigame::advance(m_game_state.pattern_state, delta_time);
        }
    }

    void GameLoop::handle_minigame_results(float delta_time)
    {
        using namespace game::player;

        bool minigame_force_walk = false;

        // Handle precision timing results
        if (!m_game_state.minigame_state.is_showing_time && 
            (game::minigame::is_success(m_game_state.minigame_state) || 
             game::minigame::is_failure(m_game_state.minigame_state)))
        {
            if (!m_game_state.precision_result_applied)
            {
                m_game_state.precision_result_applied = true;
                if (game::minigame::is_success(m_game_state.minigame_state))
                {
                    const int bonus = game::minigame::get_bonus_steps(m_game_state.minigame_state);
                    m_game_state.player_state.steps_remaining += bonus;
                    minigame_force_walk = true;
                }
                else if (game::minigame::is_failure(m_game_state.minigame_state))
                {
                    m_game_state.player_state.steps_remaining = 0;
                    m_game_state.player_state.is_stepping = false;
                }
            }

            m_game_state.precision_result_display_timer -= delta_time;
            if (m_game_state.precision_result_display_timer <= 0.0f)
            {
                game::minigame::reset(m_game_state.minigame_state);
                m_game_state.precision_result_applied = false;
                m_game_state.precision_result_display_timer = 3.0f;
            }
        }
        else
        {
            m_game_state.precision_result_applied = false;
        }

        // Handle tile memory results
        if (game::minigame::tile_memory::is_result(m_game_state.tile_memory_state))
        {
            if (!m_game_state.tile_memory_result_applied)
            {
                m_game_state.tile_memory_result_applied = true;
                if (game::minigame::tile_memory::is_success(m_game_state.tile_memory_state))
                {
                    const int bonus = game::minigame::tile_memory::get_bonus_steps(m_game_state.tile_memory_state);
                    m_game_state.player_state.steps_remaining += bonus;
                    minigame_force_walk = true;
                }
                else
                {
                    m_game_state.player_state.steps_remaining = 0;
                    m_game_state.player_state.is_stepping = false;
                }
            }
        }
        else if (!game::minigame::tile_memory::is_active(m_game_state.tile_memory_state))
        {
            m_game_state.tile_memory_result_applied = false;
        }

        // Handle reaction game results
        if (game::minigame::is_success(m_game_state.reaction_state) || 
            game::minigame::is_failure(m_game_state.reaction_state))
        {
            if (!m_game_state.reaction_result_applied)
            {
                m_game_state.reaction_result_applied = true;
                if (game::minigame::is_success(m_game_state.reaction_state))
                {
                    const int bonus = game::minigame::get_bonus_steps(m_game_state.reaction_state);
                    m_game_state.player_state.steps_remaining += bonus;
                    minigame_force_walk = true;
                }
                else
                {
                    m_game_state.player_state.steps_remaining = 0;
                    m_game_state.player_state.is_stepping = false;
                }
            }
        }
        else
        {
            m_game_state.reaction_result_applied = false;
        }

        // Handle math game results
        if (game::minigame::is_success(m_game_state.math_state) || 
            game::minigame::is_failure(m_game_state.math_state))
        {
            if (!m_game_state.math_result_applied)
            {
                m_game_state.math_result_applied = true;
                if (game::minigame::is_success(m_game_state.math_state))
                {
                    const int bonus = game::minigame::get_bonus_steps(m_game_state.math_state);
                    m_game_state.player_state.steps_remaining += bonus;
                    minigame_force_walk = true;
                }
                else
                {
                    m_game_state.player_state.steps_remaining = 0;
                    m_game_state.player_state.is_stepping = false;
                }
            }
        }
        else
        {
            m_game_state.math_result_applied = false;
        }

        // Handle pattern game results
        if (game::minigame::is_success(m_game_state.pattern_state) || 
            game::minigame::is_failure(m_game_state.pattern_state))
        {
            if (!m_game_state.pattern_result_applied)
            {
                m_game_state.pattern_result_applied = true;
                if (game::minigame::is_success(m_game_state.pattern_state))
                {
                    const int bonus = game::minigame::get_bonus_steps(m_game_state.pattern_state);
                    m_game_state.player_state.steps_remaining += bonus;
                    minigame_force_walk = true;
                }
                else
                {
                    m_game_state.player_state.steps_remaining = 0;
                    m_game_state.player_state.is_stepping = false;
                }
            }
        }
        else
        {
            m_game_state.pattern_result_applied = false;
        }
    }

    void GameLoop::render(const core::Camera& camera)
    {
        // Rendering will be handled separately
        (void)camera;
    }
}

