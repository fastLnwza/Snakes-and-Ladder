#include "game_loop.h"

#include "game/map/board.h"
#include "game/player/player.h"
#include "game/player/dice/dice.h"
#include "game/minigame/qte_minigame.h"
#include "game/minigame/tile_memory_minigame.h"
#include "game/debug/debug_warp_state.h"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <glm/glm.hpp>

namespace game
{
    namespace
    {
        void handle_dice_input(GameState& state, float delta_time)
        {
            const bool space_pressed = state.window->is_key_pressed(GLFW_KEY_SPACE);
            const bool space_just_pressed = space_pressed && !state.player_state.previous_space_state;
            const bool precision_running = game::minigame::is_running(state.minigame_state);
            const bool tile_memory_running = game::minigame::tile_memory::is_running(state.tile_memory_state);
            const bool minigame_running = precision_running || tile_memory_running;
            
            // Start dice roll when space is pressed
            if (space_just_pressed && !state.player_state.is_stepping && 
                state.player_state.steps_remaining == 0 && 
                !state.dice_state.is_rolling && 
                !state.dice_state.is_falling && 
                !minigame_running)
            {
                // Reset dice state completely
                state.dice_state.is_displaying = false;
                state.dice_state.is_rolling = false;
                state.dice_state.is_falling = false;
                state.dice_state.display_timer = 0.0f;
                state.dice_state.roll_timer = 0.0f;
                state.dice_state.result = 0;
                state.dice_state.pending_result = 0;
                
                // Calculate target position at ground level
                glm::vec3 player_pos = get_position(state.player_state);
                glm::vec3 target_pos = player_pos;
                target_pos.y = state.player_ground_y;
                
                state.dice_state.ground_y = state.player_ground_y;
                
                // Start dice roll
                const float fall_height = state.map_length * 0.4f;
                game::player::dice::start_roll(state.dice_state, target_pos, fall_height);
            }
            
            state.player_state.previous_space_state = space_pressed;
        }

        void update_dice_result(GameState& state, float delta_time)
        {
            const bool dice_ready = !state.dice_state.is_falling && 
                                   !state.dice_state.is_rolling && 
                                   state.dice_state.result > 0;
            
            // Transfer dice result to player
            if (dice_ready && state.player_state.last_dice_result != state.dice_state.result)
            {
                game::player::set_dice_result(state.player_state, state.dice_state.result);
                state.dice_display_timer = 10.0f;
            }
            
            // Update dice display timer
            if (state.dice_display_timer > 0.0f)
            {
                state.dice_display_timer = std::max(0.0f, state.dice_display_timer - delta_time);
            }
            
            // Safety check
            if (dice_ready && state.player_state.steps_remaining == 0 && state.dice_state.result > 0)
            {
                if (state.player_state.last_dice_result != state.dice_state.result)
                {
                    game::player::set_dice_result(state.player_state, state.dice_state.result);
                    state.dice_display_timer = 10.0f;
                }
            }
        }

        void update_minigames(GameState& state, float delta_time)
        {
            const bool precision_running = game::minigame::is_running(state.minigame_state);
            const bool tile_memory_running = game::minigame::tile_memory::is_running(state.tile_memory_state);
            const bool tile_memory_active = game::minigame::tile_memory::is_active(state.tile_memory_state);
            
            // Update precision timing minigame
            const bool precision_should_advance =
                precision_running || state.minigame_state.is_showing_time ||
                game::minigame::is_success(state.minigame_state) || 
                game::minigame::is_failure(state.minigame_state);
            if (precision_should_advance)
            {
                game::minigame::advance(state.minigame_state, delta_time);
            }

            if (tile_memory_active)
            {
                game::minigame::tile_memory::advance(state.tile_memory_state, delta_time);
            }
            
            // Handle precision timing input
            if (precision_running)
            {
                const bool space_down = state.window->is_key_pressed(GLFW_KEY_SPACE);
                const bool space_just_pressed = space_down && !state.precision_space_was_down;
                state.precision_space_was_down = space_down;
                
                if (space_just_pressed)
                {
                    game::minigame::stop_timing(state.minigame_state);
                }
                else if (game::minigame::has_expired(state.minigame_state))
                {
                    game::minigame::stop_timing(state.minigame_state);
                }
            }
            else
            {
                state.precision_space_was_down = false;
            }

            // Handle tile memory input
            if (tile_memory_running)
            {
                for (int key_index = 0; key_index < 9; ++key_index)
                {
                    const int glfw_key = GLFW_KEY_1 + key_index;
                    const bool key_down = state.window->is_key_pressed(glfw_key);
                    const bool key_just_pressed = key_down && !state.tile_memory_previous_keys[key_index];
                    if (key_just_pressed)
                    {
                        game::minigame::tile_memory::submit_choice(state.tile_memory_state, key_index + 1);
                    }
                    state.tile_memory_previous_keys[key_index] = key_down;
                }
            }
            else
            {
                state.tile_memory_previous_keys.fill(false);
            }
        }

        void apply_minigame_results(GameState& state, float delta_time, bool& minigame_force_walk)
        {
            // Apply precision timing result
            if (!state.minigame_state.is_showing_time && 
                (game::minigame::is_success(state.minigame_state) || 
                 game::minigame::is_failure(state.minigame_state)))
            {
                if (!state.precision_result_applied)
                {
                    state.precision_result_applied = true;
                    if (game::minigame::is_success(state.minigame_state))
                    {
                        const int bonus = game::minigame::get_bonus_steps(state.minigame_state);
                        state.player_state.steps_remaining += bonus;
                        minigame_force_walk = true;
                    }
                    else if (game::minigame::is_failure(state.minigame_state))
                    {
                        state.player_state.steps_remaining = 0;
                        state.player_state.is_stepping = false;
                    }
                }
                
                state.precision_result_display_timer -= delta_time;
                if (state.precision_result_display_timer <= 0.0f)
                {
                    game::minigame::reset(state.minigame_state);
                    state.precision_result_applied = false;
                    state.precision_result_display_timer = 3.0f;
                }
            }
            else if (state.minigame_state.is_showing_time || 
                     game::minigame::is_running(state.minigame_state))
            {
                state.precision_result_applied = false;
                state.precision_result_display_timer = 3.0f;
            }

            // Apply tile memory result
            if (game::minigame::tile_memory::is_result(state.tile_memory_state))
            {
                if (state.tile_memory_state.pending_next_round)
                {
                    state.tile_memory_result_applied = false;
                }
                else if (!state.tile_memory_result_applied)
                {
                    state.tile_memory_result_applied = true;
                    if (game::minigame::tile_memory::is_success(state.tile_memory_state))
                    {
                        const int bonus = game::minigame::tile_memory::get_bonus_steps(state.tile_memory_state);
                        state.player_state.steps_remaining += bonus;
                        minigame_force_walk = true;
                    }
                    else
                    {
                        state.player_state.steps_remaining = 0;
                        state.player_state.is_stepping = false;
                    }
                }
            }
            else if (!game::minigame::tile_memory::is_active(state.tile_memory_state))
            {
                state.tile_memory_result_applied = false;
            }
        }

        void handle_debug_warp(GameState& state)
        {
            if (state.debug_warp_state.notification_timer > 0.0f)
            {
                // Timer is updated in update_game
            }

            const bool t_key_down = state.window->is_key_pressed(GLFW_KEY_T);
            const bool precision_running = game::minigame::is_running(state.minigame_state);
            const bool tile_memory_running = game::minigame::tile_memory::is_running(state.tile_memory_state);
            const bool minigame_running = precision_running || tile_memory_running;

            if (t_key_down && !state.debug_warp_state.prev_toggle && !minigame_running)
            {
                state.debug_warp_state.active = !state.debug_warp_state.active;
                if (!state.debug_warp_state.active)
                {
                    state.debug_warp_state.buffer.clear();
                    state.debug_warp_state.digit_previous.fill(false);
                }
            }
            state.debug_warp_state.prev_toggle = t_key_down;

            if (state.debug_warp_state.active && minigame_running)
            {
                state.debug_warp_state.active = false;
                state.debug_warp_state.buffer.clear();
                state.debug_warp_state.digit_previous.fill(false);
            }

            if (state.debug_warp_state.active)
            {
                for (int digit = 0; digit <= 9; ++digit)
                {
                    const int key = GLFW_KEY_0 + digit;
                    const bool key_down = state.window->is_key_pressed(key);
                    const bool key_just_pressed = key_down && !state.debug_warp_state.digit_previous[digit];
                    if (key_just_pressed && state.debug_warp_state.buffer.size() < 3)
                    {
                        state.debug_warp_state.buffer.push_back(static_cast<char>('0' + digit));
                    }
                    state.debug_warp_state.digit_previous[digit] = key_down;
                }

                const bool backspace_down = state.window->is_key_pressed(GLFW_KEY_BACKSPACE);
                if (backspace_down && !state.debug_warp_state.prev_backspace && 
                    !state.debug_warp_state.buffer.empty())
                {
                    state.debug_warp_state.buffer.pop_back();
                }
                state.debug_warp_state.prev_backspace = backspace_down;

                const bool enter_down = state.window->is_key_pressed(GLFW_KEY_ENTER) ||
                                        state.window->is_key_pressed(GLFW_KEY_KP_ENTER);
                if (enter_down && !state.debug_warp_state.prev_enter && 
                    !state.debug_warp_state.buffer.empty())
                {
                    try
                    {
                        int requested_tile = std::stoi(state.debug_warp_state.buffer);
                        int zero_based_tile = (requested_tile <= 0) ? 0 : requested_tile - 1;
                        int target_tile = std::clamp(zero_based_tile, 0, state.final_tile_index);
                        game::player::warp_to_tile(state.player_state, target_tile);
                        state.last_processed_tile = -1;
                        state.player_state.previous_space_state = false;

                        state.dice_state.is_rolling = false;
                        state.dice_state.is_falling = false;
                        state.dice_state.is_displaying = false;
                        state.dice_state.result = 0;
                        state.dice_state.pending_result = 0;
                        state.dice_state.roll_timer = 0.0f;
                        state.dice_state.velocity = glm::vec3(0.0f);
                        state.dice_state.rotation_velocity = glm::vec3(0.0f);
                        using namespace game::map;
                        state.dice_state.position = tile_center_world(target_tile) + 
                                                    glm::vec3(0.0f, state.player_ground_y + 3.0f, 0.0f);
                        state.dice_state.target_position = state.dice_state.position;
                        state.dice_display_timer = 0.0f;

                        game::minigame::reset(state.minigame_state);
                        game::minigame::tile_memory::reset(state.tile_memory_state);
                        state.precision_result_applied = false;
                        state.tile_memory_result_applied = false;
                        state.precision_result_display_timer = 3.0f;
                        state.tile_memory_previous_keys.fill(false);
                        state.minigame_message.clear();
                        state.minigame_message_timer = 0.0f;

                        const int display_tile = target_tile + 1;
                        state.debug_warp_state.notification = "Warped to " + std::to_string(display_tile);
                        state.debug_warp_state.notification_timer = 3.0f;
                        state.debug_warp_state.buffer.clear();
                        state.debug_warp_state.active = false;
                        state.debug_warp_state.digit_previous.fill(false);
                    }
                    catch (const std::exception&)
                    {
                        state.debug_warp_state.notification = "Invalid";
                        state.debug_warp_state.notification_timer = 3.0f;
                        state.debug_warp_state.buffer.clear();
                        state.debug_warp_state.active = false;
                        state.debug_warp_state.digit_previous.fill(false);
                    }
                }
                state.debug_warp_state.prev_enter = enter_down;
            }
            else
            {
                state.debug_warp_state.digit_previous.fill(false);
                state.debug_warp_state.prev_backspace = false;
                state.debug_warp_state.prev_enter = false;
            }
        }

        void process_tile_events(GameState& state)
        {
            const int current_tile = get_current_tile(state.player_state);
            if (current_tile != state.last_processed_tile)
            {
                if (!state.player_state.is_stepping && state.player_state.steps_remaining == 0)
                {
                    int tile_to_process = current_tile;
                    
                    // Check for ladder teleportation
                    using namespace game::map;
                    const int link_destination = get_link_destination(current_tile);
                    if (link_destination >= 0)
                    {
                        game::player::warp_to_tile(state.player_state, link_destination);
                        tile_to_process = link_destination;
                        
                        // Reset dice state after teleportation
                        state.dice_state.is_rolling = false;
                        state.dice_state.is_falling = false;
                        state.dice_state.is_displaying = false;
                        state.dice_state.result = 0;
                        state.dice_state.pending_result = 0;
                        state.dice_state.roll_timer = 0.0f;
                        state.dice_state.velocity = glm::vec3(0.0f);
                        state.dice_state.rotation_velocity = glm::vec3(0.0f);
                        state.dice_state.position = tile_center_world(link_destination) + 
                                                    glm::vec3(0.0f, state.player_ground_y + 3.0f, 0.0f);
                        state.dice_state.target_position = state.dice_state.position;
                        state.dice_display_timer = 0.0f;
                    }
                    
                    state.last_processed_tile = tile_to_process;
                    
                    // Check for minigames
                    const bool precision_running = game::minigame::is_running(state.minigame_state);
                    const bool tile_memory_active = game::minigame::tile_memory::is_active(state.tile_memory_state);
                    if (!precision_running && !tile_memory_active)
                    {
                        const ActivityKind tile_activity = classify_activity_tile(tile_to_process);
                        if (tile_activity == ActivityKind::MiniGame && tile_to_process != 0)
                        {
                            game::minigame::start_precision_timing(state.minigame_state);
                            state.precision_space_was_down = false;
                            state.minigame_message = "Precision Timing Challenge! Stop at 4.99";
                            state.minigame_message_timer = 0.0f;
                        }
                        else if (tile_activity == ActivityKind::MemoryGame && tile_to_process != 0)
                        {
                            game::minigame::tile_memory::start(state.tile_memory_state);
                            state.tile_memory_previous_keys.fill(false);
                            state.minigame_message = "จำลำดับ! ใช้ปุ่ม 1-9";
                            state.minigame_message_timer = 0.0f;
                        }
                    }
                }
                else
                {
                    state.last_processed_tile = current_tile;
                }
            }
        }
    }

    void update_game(GameState& state, float delta_time)
    {
        state.window->poll_events();

        if (state.window->is_key_pressed(GLFW_KEY_ESCAPE))
        {
            state.window->close();
        }

        // Update debug warp timer
        if (state.debug_warp_state.notification_timer > 0.0f)
        {
            state.debug_warp_state.notification_timer = std::max(0.0f, 
                state.debug_warp_state.notification_timer - delta_time);
        }

        // Handle input
        handle_dice_input(state, delta_time);
        handle_debug_warp(state);

        // Update dice
        update_dice_result(state, delta_time);
        const bool dice_ready = !state.dice_state.is_falling && 
                               !state.dice_state.is_rolling && 
                               state.dice_state.result > 0;

        // Update minigames
        update_minigames(state, delta_time);

        // Apply minigame results
        bool minigame_force_walk = false;
        apply_minigame_results(state, delta_time, minigame_force_walk);

        // Update player movement
        const bool precision_running = game::minigame::is_running(state.minigame_state);
        const bool tile_memory_running = game::minigame::tile_memory::is_running(state.tile_memory_state);
        const bool minigame_running = precision_running || tile_memory_running;
        const bool dice_finished = dice_ready && !minigame_running;
        const bool has_steps_to_walk = state.player_state.steps_remaining > 0;
        const bool can_walk_now = (dice_finished && has_steps_to_walk) || minigame_force_walk;

        game::player::update(state.player_state, delta_time, false, state.final_tile_index, can_walk_now);

        // Final safeguard for walking
        if (dice_ready && state.player_state.steps_remaining > 0 && 
            !state.player_state.is_stepping && 
            !minigame_running &&
            !minigame_force_walk)
        {
            game::player::update(state.player_state, delta_time, false, state.final_tile_index, true);
        }

        // Update dice animation
        const float half_width = state.board_width * 0.5f;
        const float half_height = state.board_height * 0.5f;
        game::player::dice::update(state.dice_state, delta_time, half_width, half_height);

        // Update timers
        if (state.minigame_message_timer > 0.0f)
        {
            state.minigame_message_timer = std::max(0.0f, state.minigame_message_timer - delta_time);
        }

        // Process tile events
        process_tile_events(state);
    }
}

