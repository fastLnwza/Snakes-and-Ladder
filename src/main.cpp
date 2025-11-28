#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "core/camera.h"
#include "core/types.h"
#include "core/window.h"
#include "game/map/board.h"
#include "game/map/map_generator.h"
#include "game/minigame/qte_minigame.h"
#include "game/minigame/tile_memory_minigame.h"
#include "game/player/player.h"
#include "game/player/dice/dice.h"
#include "rendering/gltf_loader.h"
#include "rendering/obj_loader.h"
#include "rendering/mesh.h"
#include "rendering/primitives.h"
#include "rendering/shader.h"
#include "rendering/texture_loader.h"
#include "rendering/text_renderer.h"
#include "utils/file_utils.h"

int main(int argc, char* argv[])
{
    try
    {
        // Initialize window
        core::Window window(800, 600, "Pacman OpenGL");
        
        // Setup camera scroll callback
        core::Camera camera;
        window.set_scroll_callback([&camera](double /*x_offset*/, double y_offset) {
            camera.adjust_fov(static_cast<float>(y_offset));
        });

        // Load shaders
        std::filesystem::path executable_dir = std::filesystem::current_path();
        if (argc > 0)
        {
            std::error_code ec;
            auto resolved = std::filesystem::weakly_canonical(std::filesystem::path(argv[0]), ec);
            if (!ec)
            {
                executable_dir = resolved.parent_path();
            }
        }

        const auto shaders_dir = executable_dir / "shaders";
        const std::string vertex_source = load_file(shaders_dir / "simple.vert");
        const std::string fragment_source = load_file(shaders_dir / "simple.frag");
        const GLuint program = create_program(vertex_source, fragment_source);
        const GLint mvp_location = glGetUniformLocation(program, "uMVP");
        const GLint use_texture_location = glGetUniformLocation(program, "uUseTexture");
        const GLint texture_location = glGetUniformLocation(program, "uTexture");
        const GLint dice_texture_mode_location = glGetUniformLocation(program, "uDiceTextureMode");
        
        // Set texture sampler to use texture unit 0
        glUseProgram(program);
        if (texture_location >= 0)
        {
            glUniform1i(texture_location, 0); // Bind to texture unit 0
        }
        if (dice_texture_mode_location >= 0)
        {
            glUniform1i(dice_texture_mode_location, 0);
        }

        glEnable(GL_DEPTH_TEST);

        // Build map
        using namespace game::map;
        const auto [map_vertices, map_indices] = build_snakes_ladders_map();
        Mesh map_mesh = create_mesh(map_vertices, map_indices);

        const float board_width = static_cast<float>(BOARD_COLUMNS) * TILE_SIZE;
        const float board_height = static_cast<float>(BOARD_ROWS) * TILE_SIZE;
        const float map_length = board_height;
        const float map_min_dimension = std::min(board_width, board_height);

        // Create player
        using namespace game::player;
        const float player_radius = std::max(0.4f, 0.025f * map_min_dimension);
        const auto [sphere_vertices, sphere_indices] = build_sphere(player_radius, 32, 16, {1.0f, 0.9f, 0.1f});
        Mesh sphere_mesh = create_mesh(sphere_vertices, sphere_indices);

        const float player_ground_y = player_radius;
        PlayerState player_state;
        initialize(player_state, tile_center_world(0), player_ground_y, player_radius);
        const int final_tile_index = BOARD_COLUMNS * BOARD_ROWS - 1;

        // Load dice model from dice folder (support both GLB and OBJ)
        GLTFModel dice_model_glb;
        OBJModel dice_model_obj;
        bool has_dice_model = false;
        bool is_obj_format = false;
        
        try
        {
            // Calculate path to dice source folder relative to source directory
            // From src/main.cpp -> src/ -> src/game/player/dice/source/
            std::filesystem::path source_dir = std::filesystem::path(__FILE__).parent_path();
            
            // Try GLB format first
            std::filesystem::path dice_glb_path = source_dir / "game" / "player" / "dice" / "source" / "dice.glb";
            std::filesystem::path dice_obj_path = source_dir / "game" / "player" / "dice" / "source" / "dice.7z" / "dice.obj";
            
            if (std::filesystem::exists(dice_glb_path))
            {
                std::cout << "Loading dice model (GLB) from: " << dice_glb_path << std::endl;
                dice_model_glb = load_gltf_model(dice_glb_path);
                has_dice_model = true;
                is_obj_format = false;
                std::cout << "Loaded dice model with " << dice_model_glb.meshes.size() << " mesh(es)\n";
            }
            else if (std::filesystem::exists(dice_obj_path))
            {
                std::cout << "Loading dice model (OBJ) from: " << dice_obj_path << std::endl;
                dice_model_obj = load_obj_model(dice_obj_path);
                has_dice_model = true;
                is_obj_format = true;
                std::cout << "Loaded dice model with " << dice_model_obj.meshes.size() << " mesh(es)\n";
            }
            // Fallback: try executable directory
            else if (std::filesystem::exists(executable_dir / "dice.glb"))
            {
                std::cout << "Loading dice model (GLB) from executable directory\n";
                dice_model_glb = load_gltf_model(executable_dir / "dice.glb");
                has_dice_model = true;
                is_obj_format = false;
                std::cout << "Loaded dice model with " << dice_model_glb.meshes.size() << " mesh(es)\n";
            }
            else
            {
                std::cerr << "Warning: Dice model file not found\n";
                std::cerr << "Tried:\n";
                std::cerr << "  - " << dice_glb_path << "\n";
                std::cerr << "  - " << dice_obj_path << "\n";
                std::cerr << "  - " << (executable_dir / "dice.glb") << "\n";
                std::cerr << "Dice visualization will be disabled.\n";
            }
        }
        catch (const std::exception& ex)
        {
            std::cerr << "Warning: Failed to load dice model: " << ex.what() << '\n';
            std::cerr << "Dice visualization will be disabled.\n";
        }

        // Load dice texture (try multiple locations)
        Texture dice_texture{};
        bool has_dice_texture = false;
        try
        {
            std::filesystem::path source_dir = std::filesystem::path(__FILE__).parent_path();
            std::filesystem::path texture_png_path = source_dir / "game" / "player" / "dice" / "textures" / "cost.png";
            std::filesystem::path texture_dds_path = source_dir / "game" / "player" / "dice" / "source" / "dice.7z" / "cost.dds";
            
            if (std::filesystem::exists(texture_png_path))
            {
                std::cout << "Loading dice texture from: " << texture_png_path << std::endl;
                dice_texture = load_texture(texture_png_path);
                has_dice_texture = true;
                std::cout << "Dice texture loaded successfully! ID: " << dice_texture.id << std::endl;
            }
            else if (std::filesystem::exists(texture_dds_path))
            {
                std::cout << "DDS texture format detected but not supported. Please convert to PNG.\n";
                std::cout << "Found DDS at: " << texture_dds_path << std::endl;
            }
            else
            {
                std::cerr << "Warning: Dice texture not found at:\n";
                std::cerr << "  - " << texture_png_path << "\n";
                std::cerr << "  - " << texture_dds_path << "\n";
            }
        }
        catch (const std::exception& ex)
        {
            std::cerr << "Warning: Failed to load dice texture: " << ex.what() << '\n';
        }

        // Initialize dice state
        using namespace game::player::dice;
        DiceState dice_state;
        const glm::vec3 dice_position = tile_center_world(0) + glm::vec3(0.0f, player_ground_y + 3.0f, 0.0f);
        initialize(dice_state, dice_position);
        dice_state.scale = player_radius * 0.167f; // Reduced scale
        dice_state.rotation = glm::vec3(45.0f, 45.0f, 0.0f); // Initial rotation to make it visible

        // Initialize text renderer for displaying dice result
        TextRenderer text_renderer;
        std::filesystem::path font_path = executable_dir / "pixel-game.regular.otf";
        if (!std::filesystem::exists(font_path))
        {
            std::filesystem::path source_font = std::filesystem::path(__FILE__).parent_path().parent_path() / "pixel-game.regular.otf";
            if (std::filesystem::exists(source_font))
            {
                font_path = source_font;
            }
        }
        if (!std::filesystem::exists(font_path))
        {
            throw std::runtime_error("Font pixel-game.regular.otf not found.");
        }
        if (!initialize_text_renderer(text_renderer, font_path.string(), 72))
        {
            throw std::runtime_error("Failed to initialize text renderer.");
        }

        game::minigame::PrecisionTimingState minigame_state;
        game::minigame::tile_memory::TileMemoryState tile_memory_state;
        bool precision_space_was_down = false;
        std::array<bool, 9> tile_memory_previous_keys{};
        int last_processed_tile = get_current_tile(player_state);
        std::string minigame_message;
        float minigame_message_timer = 0.0f;
        float dice_display_timer = 0.0f; // Timer for showing dice result (10 seconds)

        float last_time = 0.0f;
        bool precision_result_applied = false;
        float precision_result_display_timer = 3.0f;
        bool tile_memory_result_applied = false;

        struct DebugWarpState
        {
            bool active = false;
            std::string buffer;
            std::array<bool, 10> digit_previous{};
            bool prev_toggle = false;
            bool prev_enter = false;
            bool prev_backspace = false;
            float notification_timer = 0.0f;
            std::string notification;
        };
        DebugWarpState debug_warp_state;

        // Main game loop
        last_time = static_cast<float>(glfwGetTime());
        while (!window.should_close())
        {
            const float current_time = static_cast<float>(glfwGetTime());
            const float delta_time = current_time - last_time;
            last_time = current_time;

            window.poll_events();

            if (window.is_key_pressed(GLFW_KEY_ESCAPE))
            {
                window.close();
            }

            // Update player
            const bool space_pressed = window.is_key_pressed(GLFW_KEY_SPACE);
            const bool space_just_pressed = space_pressed && !player_state.previous_space_state;
            const bool precision_running = game::minigame::is_running(minigame_state);
            const bool tile_memory_running = game::minigame::tile_memory::is_running(tile_memory_state);
            const bool tile_memory_active = game::minigame::tile_memory::is_active(tile_memory_state);
            const bool minigame_running = precision_running || tile_memory_running;
            
            // Start dice roll when space is pressed (dice will roll its own number)
            if (space_just_pressed && !player_state.is_stepping && player_state.steps_remaining == 0 && !dice_state.is_rolling && !dice_state.is_falling && !minigame_running)
            {
                // Reset dice state completely
                dice_state.is_displaying = false;
                dice_state.is_rolling = false;
                dice_state.is_falling = false;
                dice_state.display_timer = 0.0f;
                dice_state.roll_timer = 0.0f;
                dice_state.result = 0;
                dice_state.pending_result = 0;  // Clear pending result - will be set when rolling starts
                
                // Calculate target position at ground level (where dice will land)
                glm::vec3 player_pos = get_position(player_state);
                glm::vec3 target_pos = player_pos;
                target_pos.y = player_ground_y;
                
                // Set ground level for dice physics
                dice_state.ground_y = player_ground_y;
                
                // Start dice roll - dice will roll its own number randomly
                const float fall_height = map_length * 0.4f; // Fall from 40% of map height
                game::player::dice::start_roll(dice_state, target_pos, fall_height);
            }
            
            // Check if dice has finished rolling and get the result
            // When dice stops, use that result for player movement
            const bool dice_ready = !dice_state.is_falling && !dice_state.is_rolling && dice_state.result > 0;
            
            // Transfer dice result to player as soon as dice is ready
            if (dice_ready && player_state.last_dice_result != dice_state.result)
            {
                // Set the dice result to player - this is the number that was actually rolled
                game::player::set_dice_result(player_state, dice_state.result);
                // Start timer to show dice result for 10 seconds
                dice_display_timer = 10.0f;
            }
            
            // Update dice display timer
            if (dice_display_timer > 0.0f)
            {
                dice_display_timer = std::max(0.0f, dice_display_timer - delta_time);
            }
            
            // Safety check: ensure steps_remaining is set if dice is ready but result hasn't been transferred yet
            // This handles edge cases where the result check might miss
            if (dice_ready && player_state.steps_remaining == 0 && dice_state.result > 0)
            {
                if (player_state.last_dice_result != dice_state.result)
                {
                    game::player::set_dice_result(player_state, dice_state.result);
                    dice_display_timer = 10.0f;
                }
            }
            
            // Always call advance to update timers (even when showing time or result)
            const bool precision_should_advance =
                precision_running || minigame_state.is_showing_time ||
                game::minigame::is_success(minigame_state) || game::minigame::is_failure(minigame_state);
            if (precision_should_advance)
            {
                game::minigame::advance(minigame_state, delta_time);
            }

            if (tile_memory_active)
            {
                game::minigame::tile_memory::advance(tile_memory_state, delta_time);
            }
            
            if (precision_running)
            {
                // Check if SPACE is pressed to stop the timer
                const bool space_down = window.is_key_pressed(GLFW_KEY_SPACE);
                const bool space_just_pressed_for_minigame = space_down && !precision_space_was_down;
                precision_space_was_down = space_down;
                
                if (space_just_pressed_for_minigame)
                {
                    game::minigame::stop_timing(minigame_state);
                }
                else if (game::minigame::has_expired(minigame_state))
                {
                    // Time expired - automatic failure (over 10 seconds)
                    game::minigame::stop_timing(minigame_state);
                    // stop_timing will check if result is within acceptable range
                    // If timer exceeded max_time, it will be marked as failure
                }
            }
            else
            {
                precision_space_was_down = false;
            }

            if (tile_memory_running)
            {
                for (int key_index = 0; key_index < 9; ++key_index)
                {
                    const int glfw_key = GLFW_KEY_1 + key_index;
                    const bool key_down = window.is_key_pressed(glfw_key);
                    const bool key_just_pressed = key_down && !tile_memory_previous_keys[key_index];
                    if (key_just_pressed)
                    {
                        game::minigame::tile_memory::submit_choice(tile_memory_state, key_index + 1);
                    }
                    tile_memory_previous_keys[key_index] = key_down;
                }
            }
            else
            {
                tile_memory_previous_keys.fill(false);
            }

            if (debug_warp_state.notification_timer > 0.0f)
            {
                debug_warp_state.notification_timer = std::max(0.0f, debug_warp_state.notification_timer - delta_time);
            }

            const bool t_key_down = window.is_key_pressed(GLFW_KEY_T);
            if (t_key_down && !debug_warp_state.prev_toggle && !minigame_running)
            {
                debug_warp_state.active = !debug_warp_state.active;
                if (!debug_warp_state.active)
                {
                    debug_warp_state.buffer.clear();
                    debug_warp_state.digit_previous.fill(false);
                }
            }
            debug_warp_state.prev_toggle = t_key_down;

            if (debug_warp_state.active && minigame_running)
            {
                debug_warp_state.active = false;
                debug_warp_state.buffer.clear();
                debug_warp_state.digit_previous.fill(false);
            }

            if (debug_warp_state.active)
            {
                for (int digit = 0; digit <= 9; ++digit)
                {
                    const int key = GLFW_KEY_0 + digit;
                    const bool key_down = window.is_key_pressed(key);
                    const bool key_just_pressed = key_down && !debug_warp_state.digit_previous[digit];
                    if (key_just_pressed && debug_warp_state.buffer.size() < 3)
                    {
                        debug_warp_state.buffer.push_back(static_cast<char>('0' + digit));
                    }
                    debug_warp_state.digit_previous[digit] = key_down;
                }

                const bool backspace_down = window.is_key_pressed(GLFW_KEY_BACKSPACE);
                if (backspace_down && !debug_warp_state.prev_backspace && !debug_warp_state.buffer.empty())
                {
                    debug_warp_state.buffer.pop_back();
                }
                debug_warp_state.prev_backspace = backspace_down;

                const bool enter_down = window.is_key_pressed(GLFW_KEY_ENTER) ||
                                        window.is_key_pressed(GLFW_KEY_KP_ENTER);
                if (enter_down && !debug_warp_state.prev_enter && !debug_warp_state.buffer.empty())
                {
                    try
                    {
                        int requested_tile = std::stoi(debug_warp_state.buffer);
                        int zero_based_tile = (requested_tile <= 0) ? 0 : requested_tile - 1;
                        int target_tile = std::clamp(zero_based_tile, 0, final_tile_index);
                        game::player::warp_to_tile(player_state, target_tile);
                        last_processed_tile = -1;
                        player_state.previous_space_state = false;

                        dice_state.is_rolling = false;
                        dice_state.is_falling = false;
                        dice_state.is_displaying = false;
                        dice_state.result = 0;
                        dice_state.pending_result = 0;
                        dice_state.roll_timer = 0.0f;
                        dice_state.velocity = glm::vec3(0.0f);
                        dice_state.rotation_velocity = glm::vec3(0.0f);
                        dice_state.position = tile_center_world(target_tile) + glm::vec3(0.0f, player_ground_y + 3.0f, 0.0f);
                        dice_state.target_position = dice_state.position;
                        dice_display_timer = 0.0f;

                        game::minigame::reset(minigame_state);
                        game::minigame::tile_memory::reset(tile_memory_state);
                        precision_result_applied = false;
                        tile_memory_result_applied = false;
                        precision_result_display_timer = 3.0f;
                        tile_memory_previous_keys.fill(false);
                        minigame_message.clear();
                        minigame_message_timer = 0.0f;

                        const int display_tile = target_tile + 1;
                        debug_warp_state.notification = "Warped to " + std::to_string(display_tile);
                        debug_warp_state.notification_timer = 3.0f;
                        debug_warp_state.buffer.clear();
                        debug_warp_state.active = false;
                        debug_warp_state.digit_previous.fill(false);
                    }
                    catch (const std::exception&)
                    {
                        debug_warp_state.notification = "Invalid";
                        debug_warp_state.notification_timer = 3.0f;
                        debug_warp_state.buffer.clear();
                        debug_warp_state.active = false;
                        debug_warp_state.digit_previous.fill(false);
                    }
                }
                debug_warp_state.prev_enter = enter_down;
            }
            else
            {
                debug_warp_state.digit_previous.fill(false);
                debug_warp_state.prev_backspace = false;
                debug_warp_state.prev_enter = false;
            }

            bool minigame_force_walk = false;
            // Apply result once when time display finishes and result is ready
            if (!minigame_state.is_showing_time && 
                (game::minigame::is_success(minigame_state) || game::minigame::is_failure(minigame_state)))
            {
                if (!precision_result_applied)
                {
                    precision_result_applied = true;
                    if (game::minigame::is_success(minigame_state))
                    {
                        const int bonus = game::minigame::get_bonus_steps(minigame_state);
                        player_state.steps_remaining += bonus;
                        minigame_force_walk = true;
                    }
                    else if (game::minigame::is_failure(minigame_state))
                    {
                        // Failed - stay at current position, no penalty, just no bonus
                        player_state.steps_remaining = 0;
                        player_state.is_stepping = false;
                    }
                }
                
                // After showing result for 3 seconds, reset the minigame state
                precision_result_display_timer -= delta_time;
                if (precision_result_display_timer <= 0.0f)
                {
                    game::minigame::reset(minigame_state);
                    precision_result_applied = false;
                    precision_result_display_timer = 3.0f;
                }
            }
            else if (minigame_state.is_showing_time || game::minigame::is_running(minigame_state))
            {
                // Reset flag when minigame starts again
                precision_result_applied = false;
                precision_result_display_timer = 3.0f;
            }

            if (game::minigame::tile_memory::is_result(tile_memory_state))
            {
                if (tile_memory_state.pending_next_round)
                {
                    tile_memory_result_applied = false;
                }
                else if (!tile_memory_result_applied)
                {
                    tile_memory_result_applied = true;
                    if (game::minigame::tile_memory::is_success(tile_memory_state))
                    {
                        const int bonus = game::minigame::tile_memory::get_bonus_steps(tile_memory_state);
                        player_state.steps_remaining += bonus;
                        minigame_force_walk = true;
                    }
                    else
                    {
                        player_state.steps_remaining = 0;
                        player_state.is_stepping = false;
                    }
                }
            }
            else if (!game::minigame::tile_memory::is_active(tile_memory_state))
            {
                tile_memory_result_applied = false;
            }
            
            // Check if dice has finished: stopped bouncing AND result is set
            // Allow walking if:
            // 1. Dice is ready AND minigame is not currently running AND player has steps to walk
            // 2. OR minigame just finished and force walk is enabled (success case)
            bool minigame_running_check = minigame_running;
            bool dice_finished = dice_ready && !minigame_running_check;
            bool has_steps_to_walk = player_state.steps_remaining > 0;
            
            // Player can walk if:
            // - Dice finished AND has steps AND minigame not running
            // - OR minigame force walk (success case)
            bool can_walk_now = (dice_finished && has_steps_to_walk) || minigame_force_walk;
            
            // Only allow player to start walking after dice finishes bouncing and shows result
            update(player_state, delta_time, false, final_tile_index, can_walk_now);
            
            player_state.previous_space_state = space_pressed;
            
            // Final safeguard: If dice is ready, has steps, but player is not walking and not in minigame,
            // force start walking immediately - this should always trigger if conditions are met
            if (dice_ready && player_state.steps_remaining > 0 && 
                !player_state.is_stepping && 
                !minigame_running_check &&
                !minigame_force_walk)
            {
                // Force start walking - this should catch all edge cases
                update(player_state, delta_time, false, final_tile_index, true);
            }
            
            // Update dice animation with board bounds for bouncing
            const float half_width = board_width * 0.5f;
            const float half_height = board_height * 0.5f;
            update(dice_state, delta_time, half_width, half_height);

            if (minigame_message_timer > 0.0f)
            {
                minigame_message_timer = std::max(0.0f, minigame_message_timer - delta_time);
            }

            const int current_tile = get_current_tile(player_state);
            // Only trigger minigame when landing on the tile (stopped), not when passing through
            // Check if player has stopped (no steps remaining, not stepping, and tile changed)
            if (current_tile != last_processed_tile)
            {
                // Only process tile when player has stopped (not walking through)
                if (!player_state.is_stepping && player_state.steps_remaining == 0)
                {
                    last_processed_tile = current_tile;
                    if (!game::minigame::is_running(minigame_state) &&
                        !game::minigame::tile_memory::is_active(tile_memory_state))
                    {
                        const ActivityKind tile_activity = classify_activity_tile(current_tile);
                        if (tile_activity == ActivityKind::MiniGame && current_tile != 0)
                        {
                            game::minigame::start_precision_timing(minigame_state);
                            precision_space_was_down = false;
                            minigame_message = "Precision Timing Challenge! Stop at 4.99";
                            minigame_message_timer = 0.0f;
                        }
                        else if (tile_activity == ActivityKind::MemoryGame && current_tile != 0)
                        {
                            game::minigame::tile_memory::start(tile_memory_state);
                            tile_memory_previous_keys.fill(false);
                            minigame_message = "จำลำดับ! ใช้ปุ่ม 1-9";
                            minigame_message_timer = 0.0f;
                        }
                    }
                }
                else
                {
                    // Player is passing through - just update last_processed_tile without triggering events
                    last_processed_tile = current_tile;
                }
            }

            // Render
            glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            const float aspect_ratio = window.get_aspect_ratio();
            const glm::mat4 projection = camera.get_projection(aspect_ratio);
            const glm::vec3 player_position = get_position(player_state);
            const glm::mat4 view = camera.get_view(player_position, map_length);

            glUseProgram(program);
            
            // Disable texture for map and player (they use vertex colors only)
            glUniform1i(use_texture_location, 0);
            glBindTexture(GL_TEXTURE_2D, 0);

            // Draw map
            {
                const glm::mat4 model(1.0f);
                const glm::mat4 mvp = projection * view * model;
                glUniformMatrix4fv(mvp_location, 1, GL_FALSE, glm::value_ptr(mvp));
                glBindVertexArray(map_mesh.vao);
                glDrawElements(GL_TRIANGLES, map_mesh.index_count, GL_UNSIGNED_INT, nullptr);
            }

            // Draw player
            {
                const glm::mat4 model = glm::translate(glm::mat4(1.0f), player_position);
                const glm::mat4 mvp = projection * view * model;
                glUniformMatrix4fv(mvp_location, 1, GL_FALSE, glm::value_ptr(mvp));
                glBindVertexArray(sphere_mesh.vao);
                glDrawElements(GL_TRIANGLES, sphere_mesh.index_count, GL_UNSIGNED_INT, nullptr);
            }

            // Draw dice when rolling, falling, or displaying result
            if (dice_state.is_rolling || dice_state.is_falling || dice_state.is_displaying)
            {
                const std::vector<Mesh>* dice_meshes = nullptr;
                if (has_dice_model)
                {
                    if (is_obj_format)
                    {
                        if (!dice_model_obj.meshes.empty())
                            dice_meshes = &dice_model_obj.meshes;
                    }
                    else
                    {
                        if (!dice_model_glb.meshes.empty())
                            dice_meshes = &dice_model_glb.meshes;
                    }
                }
                
                if (dice_meshes && !dice_meshes->empty())
                {
                    // Use dice position - it already has offset to prevent sinking
                    glm::vec3 render_pos = dice_state.position;
                    // Dice position already accounts for scale * 1.5f offset in physics
                    // Add extra visual offset when displaying to ensure corners don't sink into tiles
                    if (dice_state.is_displaying && !dice_state.is_falling)
                    {
                        // Add extra offset to ensure dice (especially corners) don't sink
                        render_pos.y += dice_state.scale * 0.3f; // Additional visual offset to prevent corner sinking
                    }
                    
                    // Create transform with adjusted position
                    glm::vec3 original_pos = dice_state.position;
                    dice_state.position = render_pos;
                    glm::mat4 dice_transform = game::player::dice::get_transform(dice_state);
                    dice_state.position = original_pos; // Restore original position
                    
                    const glm::mat4 dice_mvp = projection * view * dice_transform;
                    
                    // Enable polygon offset to make dice render in front of decorations
                    glEnable(GL_POLYGON_OFFSET_FILL);
                    glPolygonOffset(-1.0f, -1.0f); // Negative offset to draw in front
                    
                    glUniformMatrix4fv(mvp_location, 1, GL_FALSE, glm::value_ptr(dice_mvp));
                
                // Enable texture if available
                if (has_dice_texture && dice_texture.id != 0)
                {
                    glUseProgram(program); // Make sure shader is active
                    glUniform1i(use_texture_location, 1); // Enable texture
                    if (dice_texture_mode_location >= 0)
                    {
                        glUniform1i(dice_texture_mode_location, 1);
                    }
                    glUniform1i(texture_location, 0); // Set sampler to texture unit 0
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, dice_texture.id);
                }
                else
                {
                    glUniform1i(use_texture_location, 0); // Disable texture
                }
                
                // Draw only once - use first mesh only to avoid duplicates
                if (!dice_meshes->empty())
                {
                    // Only draw the first mesh to avoid rendering duplicates
                    glBindVertexArray((*dice_meshes)[0].vao);
                    glDrawElements(GL_TRIANGLES, (*dice_meshes)[0].index_count, GL_UNSIGNED_INT, nullptr);
                }
                
                // Unbind texture
                if (has_dice_texture)
                {
                    glBindTexture(GL_TEXTURE_2D, 0);
                    if (dice_texture_mode_location >= 0)
                    {
                        glUniform1i(dice_texture_mode_location, 0);
                    }
                    glUniform1i(use_texture_location, 0);
                }
                
                // Disable polygon offset after drawing dice
                glDisable(GL_POLYGON_OFFSET_FILL);
                }
            }

            // Render UI text (dice/minigame info) in orthographic projection
            // ALWAYS show UI overlay if minigame message timer is active
            const bool show_ui_overlay = dice_state.is_displaying || dice_state.is_rolling || dice_state.is_falling ||
                                         player_state.steps_remaining > 0 || game::minigame::is_running(minigame_state) ||
                                         minigame_state.is_showing_time ||
                                         game::minigame::is_success(minigame_state) ||
                                         game::minigame::is_failure(minigame_state) ||
                                         game::minigame::tile_memory::is_active(tile_memory_state) ||
                                         debug_warp_state.active || debug_warp_state.notification_timer > 0.0f ||
                                         minigame_message_timer > 0.0f;
            
            // Force show UI overlay if message timer is active
            if (minigame_message_timer > 0.0f || show_ui_overlay)
            {
                // Get window size
                int window_width, window_height;
                window.get_framebuffer_size(&window_width, &window_height);
                
                // Switch to orthographic projection for UI
                glm::mat4 ortho_projection = glm::ortho(0.0f, static_cast<float>(window_width), 
                                                        static_cast<float>(window_height), 0.0f, -1.0f, 1.0f);
                glm::mat4 identity_view = glm::mat4(1.0f);
                glm::mat4 ui_mvp = ortho_projection * identity_view;
                
                // Set UI matrices
                glUseProgram(program);
                glUniformMatrix4fv(mvp_location, 1, GL_FALSE, glm::value_ptr(ui_mvp));
                glUniform1i(use_texture_location, 1);
                if (dice_texture_mode_location >= 0)
                {
                    glUniform1i(dice_texture_mode_location, 0);
                }
                glActiveTexture(GL_TEXTURE0);
                
                // Disable depth test for UI and enable blending for better visibility
                glDisable(GL_DEPTH_TEST);
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                
                // Render text at center of screen (top)
                float center_x = window_width * 0.5f;
                float top_y = window_height * 0.1f; // Top 10% of screen
                const float ui_primary_scale = 3.2f;
                const float ui_secondary_scale = 2.8f;

                // Priority order for UI display:
                // 1. Minigame message (highest priority) - success/failure
                // 2. Minigame running - timer display
                // 3. Dice result - only if no minigame messages
                
                // Show stopped time comparison "4.99 : X.XX" for 0.5 seconds after pressing SPACE (highest priority)
                if (minigame_state.is_showing_time)
                {
                    // Show the comparison - format: "4.99 : X.XX" at center
                    std::string time_text = game::minigame::get_display_text(minigame_state);
                    glm::vec3 timing_color = {0.9f, 0.9f, 0.3f}; // Yellow
                    render_text(text_renderer, time_text, center_x, top_y, ui_primary_scale, timing_color);
                }
                // Show result message immediately after 3 seconds (when is_showing_time becomes false)
                else if (!minigame_state.is_showing_time && 
                         (game::minigame::is_success(minigame_state) || game::minigame::is_failure(minigame_state)))
                {
                    std::string result_text = game::minigame::get_display_text(minigame_state);
                    glm::vec3 result_color;
                    if (game::minigame::is_success(minigame_state))
                    {
                        result_color = glm::vec3(0.2f, 1.0f, 0.4f); // Green for success
                    }
                    else
                    {
                        result_color = glm::vec3(1.0f, 0.3f, 0.3f); // Red for failure
                    }
                    render_text(text_renderer, result_text, center_x, top_y, ui_primary_scale, result_color);
                }
                else if (game::minigame::is_running(minigame_state))
                {
                    std::string running_text = game::minigame::get_display_text(minigame_state);
                    glm::vec3 timing_color = {0.9f, 0.9f, 0.3f};
                    render_text(text_renderer, running_text, center_x, top_y, ui_primary_scale, timing_color);
                }
                // Show minigame result message (only if not showing from state above)
                else if (game::minigame::tile_memory::is_active(tile_memory_state))
                {
                    std::string memory_text = game::minigame::tile_memory::get_display_text(tile_memory_state);
                    glm::vec3 memory_color;
                    if (game::minigame::tile_memory::is_result(tile_memory_state))
                    {
                        if (game::minigame::tile_memory::is_success(tile_memory_state))
                        {
                            memory_color = glm::vec3(0.2f, 1.0f, 0.4f);
                        }
                        else
                        {
                            memory_color = glm::vec3(1.0f, 0.3f, 0.3f);
                        }
                    }
                    else
                    {
                        memory_color = glm::vec3(0.9f, 0.9f, 0.3f);
                    }

                    render_text(text_renderer,
                                memory_text,
                                center_x,
                                top_y,
                                ui_primary_scale,
                                memory_color);
                }
                else if (debug_warp_state.active)
                {
                    std::string prompt = "Warped to ";
                    if (debug_warp_state.buffer.empty())
                    {
                        prompt += "_";
                    }
                    else
                    {
                        prompt += debug_warp_state.buffer;
                    }
                    prompt += "  [Enter]";
                    glm::vec3 debug_color = {0.3f, 0.85f, 1.0f};
                    render_text(text_renderer,
                                prompt,
                                center_x,
                                top_y,
                                ui_secondary_scale,
                                debug_color);
                }
                else if (debug_warp_state.notification_timer > 0.0f && !debug_warp_state.notification.empty())
                {
                    glm::vec3 debug_color = {0.3f, 0.85f, 1.0f};
                    render_text(text_renderer,
                                debug_warp_state.notification,
                                center_x,
                                top_y,
                                ui_secondary_scale,
                                debug_color);
                }
                else if (minigame_message_timer > 0.0f && !minigame_message.empty())
                {
                    std::string display_msg = minigame_message;
                    glm::vec3 msg_color;
                    if (display_msg.find("โบนัส") != std::string::npos || 
                        display_msg.find("+6") != std::string::npos ||
                        display_msg.find("Bonus") != std::string::npos ||
                        display_msg.find("โบน") != std::string::npos)
                    {
                        msg_color = glm::vec3(0.2f, 1.0f, 0.4f); // Green for success
                    }
                    else
                    {
                        msg_color = glm::vec3(1.0f, 0.3f, 0.3f); // Red for failure (Mission Fail)
                    }
                    
                    // Render with medium text at center of screen
                    render_text(text_renderer,
                                display_msg,
                                center_x,
                                top_y,
                                ui_primary_scale,
                                msg_color);
                }
                // Show minigame timer while running (only if no result message and not showing stopped time)
                else if (game::minigame::is_running(minigame_state))
                {
                    // Show the current time counting up - format: "4.99 : X.XX" at center
                    std::ostringstream oss;
                    oss << std::fixed << std::setprecision(2);
                    oss << "4.99 : " << minigame_state.timer;
                    std::string timing_text = oss.str();
                    
                    glm::vec3 timing_color = {0.9f, 0.9f, 0.3f}; // Yellow
                    render_text(text_renderer, timing_text, center_x, top_y, ui_secondary_scale, timing_color);
                }
                // Show dice result only if no minigame messages or minigame not running
                else if (dice_display_timer > 0.0f && dice_state.result > 0)
                {
                    std::string dice_text = std::to_string(dice_state.result);
                    glm::vec3 text_color(1.0f, 1.0f, 0.0f); // Bright yellow
                    render_text(text_renderer, dice_text, center_x, top_y, ui_primary_scale, text_color);
                }
                
                // Re-enable depth test and disable blending
                glDisable(GL_BLEND);
                glEnable(GL_DEPTH_TEST);
                glUniform1i(use_texture_location, 0);
                if (dice_texture_mode_location >= 0)
                {
                    glUniform1i(dice_texture_mode_location, 0);
                }
                glBindTexture(GL_TEXTURE_2D, 0);
            }

            window.swap_buffers();
        }

        // Cleanup
        destroy_text_renderer(text_renderer);
        if (has_dice_texture)
        {
            destroy_texture(dice_texture);
        }
        destroy_mesh(map_mesh);
        destroy_mesh(sphere_mesh);
        if (has_dice_model)
        {
            if (is_obj_format)
            {
                if (!dice_model_obj.meshes.empty())
                {
                    destroy_obj_model(dice_model_obj);
                }
            }
            else
            {
                if (!dice_model_glb.meshes.empty())
                {
                    destroy_gltf_model(dice_model_glb);
                }
            }
        }
        glDeleteProgram(program);
    }
    catch (const std::exception& ex)
    {
        std::cerr << ex.what() << '\n';
        return 1;
    }
    return 0;
}
