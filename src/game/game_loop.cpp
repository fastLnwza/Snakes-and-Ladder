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
#include <sstream>

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
        
        // Don't update game if menu or win screen is active
        if (m_game_state.menu_state.is_active || m_game_state.win_state.is_active)
        {
            return;
        }
        
        update_minigames(delta_time);
        handle_minigame_results(delta_time);
        update_game_logic(delta_time);
    }

    void GameLoop::handle_input(float delta_time)
    {
        using namespace game::player;
        using namespace game::player::dice;
        using namespace game::map;

        // Handle menu input
        // Static variables for menu key debouncing (shared across menu active/inactive)
        static bool menu_up_was_pressed = false;
        static bool menu_down_was_pressed = false;
        static bool menu_left_was_pressed = false;
        static bool menu_right_was_pressed = false;
        static bool menu_enter_was_pressed = false;
        static bool menu_space_was_pressed = false;
        static bool menu_was_active = false;
        
        if (m_game_state.menu_state.is_active)
        {
            // Reset key states if menu just became active (to prevent key carryover from previous screen)
            if (!menu_was_active)
            {
                // When menu just became active, mark all currently pressed keys as "already pressed"
                // This forces user to release and press again, preventing key carryover
                // This is especially important for Space key from win screen
                menu_up_was_pressed = m_window.is_key_pressed(GLFW_KEY_UP) || m_window.is_key_pressed(GLFW_KEY_W);
                menu_down_was_pressed = m_window.is_key_pressed(GLFW_KEY_DOWN) || m_window.is_key_pressed(GLFW_KEY_S);
                menu_left_was_pressed = m_window.is_key_pressed(GLFW_KEY_LEFT) || m_window.is_key_pressed(GLFW_KEY_A);
                menu_right_was_pressed = m_window.is_key_pressed(GLFW_KEY_RIGHT) || m_window.is_key_pressed(GLFW_KEY_D);
                menu_enter_was_pressed = m_window.is_key_pressed(GLFW_KEY_ENTER);
                menu_space_was_pressed = m_window.is_key_pressed(GLFW_KEY_SPACE);
            }
            menu_was_active = true;
            
            const bool up_pressed = m_window.is_key_pressed(GLFW_KEY_UP) || m_window.is_key_pressed(GLFW_KEY_W);
            const bool down_pressed = m_window.is_key_pressed(GLFW_KEY_DOWN) || m_window.is_key_pressed(GLFW_KEY_S);
            const bool left_pressed = m_window.is_key_pressed(GLFW_KEY_LEFT) || m_window.is_key_pressed(GLFW_KEY_A);
            const bool right_pressed = m_window.is_key_pressed(GLFW_KEY_RIGHT) || m_window.is_key_pressed(GLFW_KEY_D);
            const bool enter_pressed = m_window.is_key_pressed(GLFW_KEY_ENTER);
            const bool space_pressed = m_window.is_key_pressed(GLFW_KEY_SPACE);
            
            // Navigate menu options (only between Players and AI, not Start button)
            if (up_pressed && !menu_up_was_pressed)
            {
                m_game_state.menu_state.selected_option = (m_game_state.menu_state.selected_option - 1 + 2) % 2;
            }
            if (down_pressed && !menu_down_was_pressed)
            {
                m_game_state.menu_state.selected_option = (m_game_state.menu_state.selected_option + 1) % 2;
            }
            
            // Change values for selected option
            if (m_game_state.menu_state.selected_option == 0)
            {
                // Number of players
                if (left_pressed && !menu_left_was_pressed)
                {
                    m_game_state.menu_state.num_players = std::max(1, m_game_state.menu_state.num_players - 1);
                }
                if (right_pressed && !menu_right_was_pressed)
                {
                    m_game_state.menu_state.num_players = std::min(4, m_game_state.menu_state.num_players + 1);
                }
            }
            else if (m_game_state.menu_state.selected_option == 1)
            {
                // AI toggle
                if ((left_pressed && !menu_left_was_pressed) || (right_pressed && !menu_right_was_pressed) || 
                    (enter_pressed && !menu_enter_was_pressed))
                {
                    m_game_state.menu_state.use_ai = !m_game_state.menu_state.use_ai;
                }
            }
            
            // Start game with Space (regardless of selected option)
            // Enter can also start game if not on AI option (Enter on AI option toggles AI)
            // Only start if Space was just pressed (not held down from previous screen)
            if ((space_pressed && !menu_space_was_pressed) || (enter_pressed && !menu_enter_was_pressed && m_game_state.menu_state.selected_option == 0))
            {
                m_game_state.menu_state.is_active = false;
                m_game_state.menu_state.start_game = true;
            }
            
            menu_up_was_pressed = up_pressed;
            menu_down_was_pressed = down_pressed;
            menu_left_was_pressed = left_pressed;
            menu_right_was_pressed = right_pressed;
            menu_enter_was_pressed = enter_pressed;
            menu_space_was_pressed = space_pressed;
            
            return;
        }
        else
        {
            // Mark menu as inactive when not active
            menu_was_active = false;
        }

        // Handle win screen input - return to menu with Space
        if (m_game_state.win_state.is_active && m_game_state.win_state.show_animation)
        {
            // Update animation timer (needed for animation even though game logic doesn't update)
            m_game_state.win_state.animation_timer += delta_time;
            
            const bool space_pressed = m_window.is_key_pressed(GLFW_KEY_SPACE);
            const bool space_just_pressed = space_pressed && !m_game_state.win_state.previous_space_state;
            if (space_just_pressed)
            {
                // Return to main menu
                m_game_state.win_state.is_active = false;
                m_game_state.win_state.show_animation = false;
                m_game_state.menu_state.is_active = true;
                // Reset game state completely
                game::player::warp_to_tile(m_game_state.player_state, 0);
                m_game_state.player_state.steps_remaining = 0;
                m_game_state.player_state.is_stepping = false;
                m_game_state.last_processed_tile = 0;
                
                // Reset all minigames
                game::minigame::reset(m_game_state.minigame_state);
                game::minigame::tile_memory::reset(m_game_state.tile_memory_state);
                game::minigame::reset(m_game_state.reaction_state);
                game::minigame::reset(m_game_state.math_state);
                game::minigame::reset(m_game_state.pattern_state);
                
                // Reset dice state
                m_game_state.dice_state.is_rolling = false;
                m_game_state.dice_state.is_falling = false;
                m_game_state.dice_state.is_displaying = false;
                m_game_state.dice_state.result = 0;
                m_game_state.dice_state.pending_result = 0;
                m_game_state.dice_state.roll_timer = 0.0f;
                m_game_state.dice_display_timer = 0.0f;
                
                // Reset all key states
                m_game_state.tile_memory_previous_keys.fill(false);
                m_game_state.tile_memory_backspace_was_down = false;
                m_game_state.tile_memory_enter_was_down = false;
                m_game_state.precision_space_was_down = false;
                m_game_state.reaction_backspace_was_down = false;
                m_game_state.reaction_enter_was_down = false;
                m_game_state.math_backspace_was_down = false;
                m_game_state.math_enter_was_down = false;
                m_game_state.math_digit_previous.fill(false);
                m_game_state.pattern_previous_keys.fill(false);
                
                // Reset result flags
                m_game_state.precision_result_applied = false;
                m_game_state.tile_memory_result_applied = false;
                m_game_state.reaction_result_applied = false;
                m_game_state.math_result_applied = false;
                m_game_state.pattern_result_applied = false;
                m_game_state.minigame_message.clear();
                m_game_state.minigame_message_timer = 0.0f;
            }
            m_game_state.win_state.previous_space_state = space_pressed;
            return;
        }
        else if (!m_game_state.win_state.is_active)
        {
            m_game_state.win_state.previous_space_state = false;
        }

        const bool space_pressed = m_window.is_key_pressed(GLFW_KEY_SPACE);
        const bool space_just_pressed = space_pressed && !m_game_state.player_state.previous_space_state;
        
        // Handle minigame title screen - wait for Space to start
        if (m_game_state.minigame_state.status == game::minigame::PrecisionTimingStatus::ShowingTitle)
        {
            if (space_just_pressed)
            {
                m_game_state.minigame_state.status = game::minigame::PrecisionTimingStatus::Running;
                m_game_state.minigame_state.title_timer = m_game_state.minigame_state.title_duration; // Skip title timer
                m_game_state.minigame_state.display_text = "Press SPACE to stop at 4.99!"; // Clear title/bonus text
                // Reset precision_space_was_down to prevent the Space key used to start the game
                // from being counted as stopping the timer
                m_game_state.precision_space_was_down = true; // Mark Space as already pressed
            }
            return;
        }
        else if (m_game_state.tile_memory_state.phase == game::minigame::tile_memory::Phase::ShowingTitle)
        {
            if (space_just_pressed)
            {
                // The state is already initialized by start(), we just need to transition to the first round
                // We'll manually trigger the advance() logic that would normally happen after title_duration
                // This will call start_round internally
                m_game_state.tile_memory_state.title_timer = m_game_state.tile_memory_state.title_duration; // Skip title timer
                // Reset key states to prevent multiple inputs
                m_game_state.tile_memory_previous_keys.fill(false);
                // Let advance() handle the transition - it will check title_timer >= title_duration
            }
            return;
        }
        else if (m_game_state.reaction_state.phase == game::minigame::ReactionState::Phase::ShowingTitle)
        {
            if (space_just_pressed)
            {
                // Start game immediately when Space is pressed - skip InitialMessage phase
                m_game_state.reaction_state.phase = game::minigame::ReactionState::Phase::PlayerTurn;
                m_game_state.reaction_state.timer = 0.0f;
                m_game_state.reaction_state.title_timer = m_game_state.reaction_state.title_duration; // Skip title timer
                m_game_state.reaction_state.player_attempts = 0;
                m_game_state.reaction_state.input_buffer.clear();
                // Set display text for player input
                std::ostringstream ss;
                ss << "Guess " << (m_game_state.reaction_state.player_attempts + 1) << "/" << m_game_state.reaction_state.max_attempts << " : input _\n(space)";
                m_game_state.reaction_state.display_text = ss.str();
            }
            return;
        }
        else if (m_game_state.math_state.phase == game::minigame::MathQuizState::Phase::ShowingTitle)
        {
            if (space_just_pressed)
            {
                m_game_state.math_state.phase = game::minigame::MathQuizState::Phase::ShowingQuestion;
                m_game_state.math_state.timer = 0.0f;
                m_game_state.math_state.title_timer = m_game_state.math_state.title_duration; // Skip title timer
                m_game_state.math_state.display_text.clear(); // Clear title/bonus text
            }
            return;
        }
        else if (m_game_state.pattern_state.phase == game::minigame::PatternMatchingState::Phase::ShowingTitle)
        {
            if (space_just_pressed)
            {
                m_game_state.pattern_state.phase = game::minigame::PatternMatchingState::Phase::ShowingPattern;
                m_game_state.pattern_state.show_timer = 0.0f;
                m_game_state.pattern_state.title_timer = m_game_state.pattern_state.title_duration; // Skip title timer
                // Generate pattern text using key letters (W S A D) that match the input keys
                std::ostringstream oss;
                for (int i = 0; i < 4; ++i)
                {
                    const char* dirs[] = {"", "W", "S", "A", "D"};
                    oss << dirs[m_game_state.pattern_state.pattern[i]];
                    if (i < 3) oss << " ";
                }
                m_game_state.pattern_state.display_text = oss.str(); // Set pattern text, clears title/bonus
            }
            return;
        }
        
        const bool precision_running = game::minigame::is_running(m_game_state.minigame_state);
        const bool tile_memory_running = game::minigame::tile_memory::is_running(m_game_state.tile_memory_state);
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

        // Handle tile memory input (uses buffer with Enter/Space to submit)
        static game::minigame::tile_memory::Phase previous_tile_memory_phase = game::minigame::tile_memory::Phase::Inactive;
        static float tile_memory_key_cooldown[9] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
        const float KEY_COOLDOWN_TIME = 0.15f; // 150ms cooldown to prevent key repeat
        
        if (tile_memory_running)
        {
            const auto current_phase = m_game_state.tile_memory_state.phase;
            
            // Check if phase just changed to WaitingInput - reset key states immediately
            if (current_phase == game::minigame::tile_memory::Phase::WaitingInput && 
                previous_tile_memory_phase != game::minigame::tile_memory::Phase::WaitingInput)
            {
                // Phase just changed to WaitingInput
                // Mark all currently pressed keys as "already pressed" to force release before accepting input
                // This prevents keys held from previous phase from triggering actions
                for (int key_index = 0; key_index < 9; ++key_index)
                {
                    const int glfw_key = GLFW_KEY_1 + key_index;
                    // Set to true if key is currently pressed - user must release and press again
                    m_game_state.tile_memory_previous_keys[key_index] = m_window.is_key_pressed(glfw_key);
                    // Reset cooldown when phase changes
                    tile_memory_key_cooldown[key_index] = 0.0f;
                }
                // Same for backspace and enter/space - mark as pressed if currently down
                m_game_state.tile_memory_backspace_was_down = m_window.is_key_pressed(GLFW_KEY_BACKSPACE);
                m_game_state.tile_memory_enter_was_down = m_window.is_key_pressed(GLFW_KEY_SPACE) || 
                                                          m_window.is_key_pressed(GLFW_KEY_ENTER) ||
                                                          m_window.is_key_pressed(GLFW_KEY_KP_ENTER);
            }
            
            previous_tile_memory_phase = current_phase;
            
            // Only accept input when in WaitingInput phase
            if (current_phase == game::minigame::tile_memory::Phase::WaitingInput)
            {
                // Update cooldown timers
                for (int key_index = 0; key_index < 9; ++key_index)
                {
                    if (tile_memory_key_cooldown[key_index] > 0.0f)
                    {
                        tile_memory_key_cooldown[key_index] -= delta_time;
                        if (tile_memory_key_cooldown[key_index] < 0.0f)
                        {
                            tile_memory_key_cooldown[key_index] = 0.0f;
                        }
                    }
                }
                
                // Handle digit input (1-9)
                // Use strict edge detection with cooldown: only trigger when key transitions from released to pressed
                // AND cooldown has expired to prevent key repeat
                bool processed_digit_this_frame = false;
                
                // First pass: detect rising edges and process ONE key press
                for (int key_index = 0; key_index < 9; ++key_index)
                {
                    if (processed_digit_this_frame)
                        break; // Only process one key per frame
                        
                    const int glfw_key = GLFW_KEY_1 + key_index;
                    const bool key_down_now = m_window.is_key_pressed(glfw_key);
                    const bool key_down_before = m_game_state.tile_memory_previous_keys[key_index];
                    const bool cooldown_expired = tile_memory_key_cooldown[key_index] <= 0.0f;
                    
                    // Strict rising edge detection with cooldown: 
                    // 1. Key was NOT pressed before (key_down_before == false) - ensures key was released
                    // 2. Key IS pressed now (key_down_now == true) - detects new press
                    // 3. Cooldown has expired - prevents rapid key repeat
                    if (key_down_now && !key_down_before && cooldown_expired)
                    {
                        // Add digit only once per key press
                        game::minigame::tile_memory::add_digit(m_game_state.tile_memory_state, static_cast<char>('1' + key_index));
                        // Mark as processed to prevent processing other keys in same frame
                        processed_digit_this_frame = true;
                        // Set cooldown to prevent immediate repeat
                        tile_memory_key_cooldown[key_index] = KEY_COOLDOWN_TIME;
                        // Immediately mark this key as pressed to prevent repeat in same frame
                        m_game_state.tile_memory_previous_keys[key_index] = true;
                    }
                }
                
                // Second pass: update ALL key states AFTER processing
                // This is critical - we must update states for all keys to track their current state
                for (int key_index = 0; key_index < 9; ++key_index)
                {
                    const int glfw_key = GLFW_KEY_1 + key_index;
                    const bool key_down_now = m_window.is_key_pressed(glfw_key);
                    
                    // Update state - if key is released, reset cooldown
                    if (!key_down_now)
                    {
                        // Key is released - reset cooldown and update state
                        tile_memory_key_cooldown[key_index] = 0.0f;
                        m_game_state.tile_memory_previous_keys[key_index] = false;
                    }
                    else if (!processed_digit_this_frame || !m_game_state.tile_memory_previous_keys[key_index])
                    {
                        // Update state for keys that weren't just processed
                        m_game_state.tile_memory_previous_keys[key_index] = key_down_now;
                    }
                }

                // Handle Backspace
                const bool backspace_down = m_window.is_key_pressed(GLFW_KEY_BACKSPACE);
                if (backspace_down && !m_game_state.tile_memory_backspace_was_down)
                {
                    game::minigame::tile_memory::remove_digit(m_game_state.tile_memory_state);
                }
                m_game_state.tile_memory_backspace_was_down = backspace_down;

                // Handle Enter/Space to submit
                // Only submit if buffer is not empty to prevent accidental submission
                const bool space_down = m_window.is_key_pressed(GLFW_KEY_SPACE);
                const bool enter_down = m_window.is_key_pressed(GLFW_KEY_ENTER) ||
                                        m_window.is_key_pressed(GLFW_KEY_KP_ENTER);
                if ((space_down || enter_down) && !m_game_state.tile_memory_enter_was_down)
                {
                    // Only submit if buffer has content
                    if (!m_game_state.tile_memory_state.input_buffer.empty())
                    {
                        game::minigame::tile_memory::submit_buffer(m_game_state.tile_memory_state);
                    }
                }
                m_game_state.tile_memory_enter_was_down = space_down || enter_down;
            }
            else
            {
                // Reset all key states when not in WaitingInput phase to prevent key repeat
                m_game_state.tile_memory_previous_keys.fill(false);
                m_game_state.tile_memory_backspace_was_down = false;
                m_game_state.tile_memory_enter_was_down = false;
            }
        }
        else
        {
            previous_tile_memory_phase = game::minigame::tile_memory::Phase::Inactive;
            m_game_state.tile_memory_previous_keys.fill(false);
            m_game_state.tile_memory_backspace_was_down = false;
            m_game_state.tile_memory_enter_was_down = false;
            // Reset cooldowns when game is not running
            for (int key_index = 0; key_index < 9; ++key_index)
            {
                tile_memory_key_cooldown[key_index] = 0.0f;
            }
        }

        // Handle reaction game input (Number Guessing - uses buffer with Enter/Space to submit)
        if (reaction_running)
        {
            // Handle digit input (1-9)
            for (int digit = 1; digit <= 9; ++digit)
            {
                const int key = GLFW_KEY_0 + digit;
                const bool key_down = m_window.is_key_pressed(key);
                const bool key_just_pressed = key_down && !m_game_state.tile_memory_previous_keys[digit];
                if (key_just_pressed)
                {
                    game::minigame::add_digit(m_game_state.reaction_state, static_cast<char>('0' + digit));
                }
                m_game_state.tile_memory_previous_keys[digit] = key_down;
            }

            // Handle Backspace
            const bool backspace_down = m_window.is_key_pressed(GLFW_KEY_BACKSPACE);
            if (backspace_down && !m_game_state.reaction_backspace_was_down)
            {
                game::minigame::remove_digit(m_game_state.reaction_state);
            }
            m_game_state.reaction_backspace_was_down = backspace_down;

            // Handle Enter/Space to submit
            const bool space_down = m_window.is_key_pressed(GLFW_KEY_SPACE);
            const bool enter_down = m_window.is_key_pressed(GLFW_KEY_ENTER) ||
                                    m_window.is_key_pressed(GLFW_KEY_KP_ENTER);
            if ((space_down || enter_down) && !m_game_state.reaction_enter_was_down)
            {
                game::minigame::submit_buffer(m_game_state.reaction_state);
            }
            m_game_state.reaction_enter_was_down = space_down || enter_down;
        }
        else
        {
            // Reset key states when game is not running
            for (int digit = 1; digit <= 9; ++digit)
            {
                m_game_state.tile_memory_previous_keys[digit] = false;
            }
            m_game_state.reaction_backspace_was_down = false;
            m_game_state.reaction_enter_was_down = false;
        }

        // Handle math game input (multi-digit)
        if (math_running)
        {
            for (int digit = 0; digit <= 9; ++digit)
            {
                const int key = GLFW_KEY_0 + digit;
                const bool key_down = m_window.is_key_pressed(key);
                const bool key_just_pressed = key_down && !m_game_state.math_digit_previous[digit];
                if (key_just_pressed && m_game_state.math_state.input_buffer.size() < 3)
                {
                    game::minigame::add_digit(m_game_state.math_state, static_cast<char>('0' + digit));
                }
                m_game_state.math_digit_previous[digit] = key_down;
            }

            const bool delete_down = m_window.is_key_pressed(GLFW_KEY_DELETE);
            const bool backspace_down = m_window.is_key_pressed(GLFW_KEY_BACKSPACE);
            if ((delete_down || backspace_down) && !m_game_state.math_backspace_was_down)
            {
                game::minigame::remove_digit(m_game_state.math_state);
            }
            m_game_state.math_backspace_was_down = delete_down || backspace_down;

            const bool space_down = m_window.is_key_pressed(GLFW_KEY_SPACE);
            const bool enter_down = m_window.is_key_pressed(GLFW_KEY_ENTER) ||
                                    m_window.is_key_pressed(GLFW_KEY_KP_ENTER);
            if ((space_down || enter_down) && !m_game_state.math_enter_was_down)
            {
                game::minigame::submit_buffer(m_game_state.math_state);
            }
            m_game_state.math_enter_was_down = space_down || enter_down;
        }
        else
        {
            // Reset key states when game is not running
            m_game_state.math_digit_previous.fill(false);
            m_game_state.math_backspace_was_down = false;
            m_game_state.math_enter_was_down = false;
        }

        // Handle pattern game input
        if (pattern_running)
        {
            // Handle character input (W, S, A, D)
            const bool w_down = m_window.is_key_pressed(GLFW_KEY_W);
            const bool s_down = m_window.is_key_pressed(GLFW_KEY_S);
            const bool a_down = m_window.is_key_pressed(GLFW_KEY_A);
            const bool d_down = m_window.is_key_pressed(GLFW_KEY_D);

            if (w_down && !m_game_state.pattern_previous_keys[0])
            {
                game::minigame::add_char_input(m_game_state.pattern_state, 'W');
            }
            if (s_down && !m_game_state.pattern_previous_keys[1])
            {
                game::minigame::add_char_input(m_game_state.pattern_state, 'S');
            }
            if (a_down && !m_game_state.pattern_previous_keys[2])
            {
                game::minigame::add_char_input(m_game_state.pattern_state, 'A');
            }
            if (d_down && !m_game_state.pattern_previous_keys[3])
            {
                game::minigame::add_char_input(m_game_state.pattern_state, 'D');
            }

            m_game_state.pattern_previous_keys[0] = w_down;
            m_game_state.pattern_previous_keys[1] = s_down;
            m_game_state.pattern_previous_keys[2] = a_down;
            m_game_state.pattern_previous_keys[3] = d_down;

            // Handle Backspace to delete
            const bool backspace_down = m_window.is_key_pressed(GLFW_KEY_BACKSPACE);
            if (backspace_down && !m_game_state.pattern_previous_keys[4])
            {
                game::minigame::delete_char(m_game_state.pattern_state);
            }
            m_game_state.pattern_previous_keys[4] = backspace_down;

            // Handle Enter/Space to submit
            const bool enter_down = m_window.is_key_pressed(GLFW_KEY_ENTER) || 
                                    m_window.is_key_pressed(GLFW_KEY_KP_ENTER);
            const bool space_down = m_window.is_key_pressed(GLFW_KEY_SPACE);
            if ((enter_down || space_down) && !m_game_state.pattern_previous_keys[5])
            {
                game::minigame::submit_answer(m_game_state.pattern_state);
            }
            m_game_state.pattern_previous_keys[5] = (enter_down || space_down);
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
                    m_game_state.precision_result_display_timer = 5.0f;
                    m_game_state.tile_memory_previous_keys.fill(false);
                    m_game_state.minigame_message.clear();
                    m_game_state.minigame_message_timer = 0.0f;

                    // Clear notification immediately - don't show message after warp
                    m_game_state.debug_warp_state.notification.clear();
                    m_game_state.debug_warp_state.notification_timer = 0.0f;
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
            m_game_state.dice_display_timer = 3.0f;
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
                m_game_state.dice_display_timer = 3.0f;
            }
        }

        // Update player movement
        const bool precision_running = game::minigame::is_running(m_game_state.minigame_state);
        const bool tile_memory_running = game::minigame::tile_memory::is_running(m_game_state.tile_memory_state);
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

                // Check for win condition
                if (current_tile >= m_game_state.final_tile_index && !m_game_state.win_state.is_active)
                {
                    m_game_state.win_state.is_active = true;
                    m_game_state.win_state.show_animation = true;
                    m_game_state.win_state.animation_timer = 0.0f;
                    m_game_state.win_state.winner_player = 1; // For now, always player 1
                }

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

        // Note: Win screen animation timer and input handling are now in handle_input()
        // to ensure they work even when update_game_logic() is skipped
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

        if (game::minigame::is_running(m_game_state.reaction_state) || 
            game::minigame::is_failure(m_game_state.reaction_state) ||
            game::minigame::is_success(m_game_state.reaction_state))
        {
            game::minigame::advance(m_game_state.reaction_state, delta_time);
        }

        if (game::minigame::is_running(m_game_state.math_state) ||
            game::minigame::is_success(m_game_state.math_state) ||
            game::minigame::is_failure(m_game_state.math_state))
        {
            game::minigame::advance(m_game_state.math_state, delta_time);
        }

        if (game::minigame::is_running(m_game_state.pattern_state) ||
            game::minigame::is_success(m_game_state.pattern_state) ||
            game::minigame::is_failure(m_game_state.pattern_state))
        {
            game::minigame::advance(m_game_state.pattern_state, delta_time);
        }
    }

    void GameLoop::handle_minigame_results(float delta_time)
    {
        using namespace game::player;

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
                m_game_state.precision_result_display_timer = 2.0f;
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

