#include "renderer.h"

#include "../game/map/map_manager.h"
#include "../game/player/player.h"
#include "../game/player/dice/dice.h"
#include "../game/minigame/qte_minigame.h"
#include "../game/minigame/tile_memory_minigame.h"
#include "../game/minigame/reaction_minigame.h"
#include "../game/minigame/math_minigame.h"
#include "../game/minigame/pattern_minigame.h"
#include "../game/menu/menu_renderer.h"
#include "../game/win/win_renderer.h"
#include "../game/minigame/minigame_menu_renderer.h"
#include "../rendering/text_renderer.h"
#include "../rendering/mesh.h"
#include "../rendering/animation_player.h"

#include <glad/glad.h>
#include <iomanip>
#include <sstream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <string>

namespace game
{
    Renderer::Renderer(const RenderState& render_state)
        : m_render_state(render_state)
    {
    }

    void Renderer::render(const core::Window& window, const core::Camera& camera, const GameState& game_state)
    {
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const float aspect_ratio = window.get_aspect_ratio();
        const glm::mat4 projection = camera.get_projection(aspect_ratio);
        // Use current player's position for camera
        const auto& current_player = game_state.players[game_state.current_player_index];
        const glm::vec3 camera_position = get_position(current_player);
        const glm::mat4 view = camera.get_view(camera_position, game_state.map_length);

        glUseProgram(m_render_state.program);
        glUniform1i(m_render_state.use_texture_location, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        // Reset color override at the start of each frame
        if (m_render_state.use_color_override_location >= 0)
        {
            glUniform1i(m_render_state.use_color_override_location, 0);
        }

        render_map(projection, view, game_state);
        render_player(projection, view, game_state);
        render_dice(projection, view, game_state);
        render_ui(window, game_state);

        // Render menu popup on top if active (transparent background, shows map behind)
        if (game_state.win_state.is_active)
        {
            game::win::render_win_screen(window, &m_render_state, game_state.win_state);
        }
        else if (game_state.menu_state.is_active)
        {
            game::menu::render_menu(window, &m_render_state, game_state.menu_state);
        }
    }

    void Renderer::render_map(const glm::mat4& projection, const glm::mat4& view, const GameState& game_state)
    {
        game::map::render_map(game_state.map_data, projection, view, m_render_state.program, m_render_state.mvp_location);
    }

    namespace
    {
        // Helper function to apply animation transforms to model matrix
        void apply_animation_transform(glm::mat4& model, const AnimationPlayerState& anim_state)
        {
            if (animation_player::is_playing(anim_state))
            {
                // Try to get root node transform (common node names: "Root", "root", "Armature", etc.)
                std::vector<std::string> root_node_names = {"Root", "root", "Armature", "armature", "Scene", "scene"};
                for (const auto& node_name : root_node_names)
                {
                    glm::mat4 node_transform = animation_player::get_node_transform(anim_state, node_name);
                    if (node_transform != glm::mat4(1.0f))
                    {
                        model = model * node_transform;
                        return;
                    }
                }
                
                // If no root node found, try to apply first available transform
                // This is a simplified approach - full implementation would use scene graph
                for (const auto& [node_name, transform] : anim_state.node_transforms)
                {
                    model = model * transform;
                    break; // Use first transform found
                }
            }
        }
    }

    void Renderer::render_player(const glm::mat4& projection, const glm::mat4& view, const GameState& game_state)
    {
        // Render all active players
        for (int i = 0; i < game_state.num_players; ++i)
        {
            const glm::vec3 player_position = get_position(game_state.players[i]);
            
            // Determine which model to use: player4 (GLB) for index 3, player3 (GLB) for index 2, player2 (GLB) for index 1, player1 (GLB) for others
            bool use_player4 = (i == 3 && game_state.has_player4_model && !game_state.player4_model_glb.meshes.empty());
            bool use_player3 = (!use_player4 && i == 2 && game_state.has_player3_model && !game_state.player3_model_glb.meshes.empty());
            bool use_player2 = (!use_player4 && !use_player3 && i == 1 && game_state.has_player2_model && !game_state.player2_model_glb.meshes.empty());
            bool use_player1 = (!use_player4 && !use_player3 && !use_player2 && game_state.has_player_model && !game_state.player_model_glb.meshes.empty());
            
            if (use_player4)
            {
                // Render player4 using GLB model (same as player1)
                const GLTFModel& model_to_use = game_state.player4_model_glb;
                
                // Scale and position - same as player1
                const float model_scale = game_state.player_radius * 2.0f;
                
                // Apply transforms - same as player1
                glm::mat4 model = glm::translate(glm::mat4(1.0f), player_position);
                model = model * glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // Flip if upside down
                model = model * glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // Stand up
                model = model * glm::scale(glm::mat4(1.0f), glm::vec3(model_scale));
                model = model * model_to_use.base_transform;
                
                // Apply animation transforms if available
                apply_animation_transform(model, game_state.player_animations[i]);
                
                const glm::mat4 mvp = projection * view * model;
                glUniformMatrix4fv(m_render_state.mvp_location, 1, GL_FALSE, glm::value_ptr(mvp));
                
                // Render all meshes in the model
                for (size_t mesh_idx = 0; mesh_idx < model_to_use.meshes.size(); ++mesh_idx)
                {
                    const auto& mesh = model_to_use.meshes[mesh_idx];
                    
                    // Use texture if available (use first texture for now, or match to mesh)
                    bool has_texture = !model_to_use.textures.empty();
                    if (has_texture)
                    {
                        size_t texture_idx = (mesh_idx < model_to_use.textures.size()) ? mesh_idx : 0;
                        const auto& texture = model_to_use.textures[texture_idx];
                        
                        if (texture.id != 0)
                        {
                            glUniform1i(m_render_state.use_texture_location, 1);
                            glActiveTexture(GL_TEXTURE0);
                            glBindTexture(GL_TEXTURE_2D, texture.id);
                        }
                        else
                        {
                            has_texture = false;
                        }
                    }
                    
                    if (!has_texture)
                    {
                        // No texture, use vertex colors (which should be loaded from model)
                        glUniform1i(m_render_state.use_texture_location, 0);
                        glBindTexture(GL_TEXTURE_2D, 0);
                    }
                    
                    glBindVertexArray(mesh.vao);
                    glDrawElements(GL_TRIANGLES, mesh.index_count, GL_UNSIGNED_INT, nullptr);
                }
                
                // Reset texture state
                glUniform1i(m_render_state.use_texture_location, 0);
                glBindTexture(GL_TEXTURE_2D, 0);
            }
            else if (use_player3)
            {
                // Render player3 using GLB model (same as player1)
                const GLTFModel& model_to_use = game_state.player3_model_glb;
                
                // Scale and position - same as player1
                const float model_scale = game_state.player_radius * 2.0f;
                
                // Apply transforms - same as player1
                glm::mat4 model = glm::translate(glm::mat4(1.0f), player_position);
                model = model * glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // Flip if upside down
                model = model * glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // Stand up
                model = model * glm::scale(glm::mat4(1.0f), glm::vec3(model_scale));
                model = model * model_to_use.base_transform;
                
                // Apply animation transforms if available
                apply_animation_transform(model, game_state.player_animations[i]);
                
                const glm::mat4 mvp = projection * view * model;
                glUniformMatrix4fv(m_render_state.mvp_location, 1, GL_FALSE, glm::value_ptr(mvp));
                
                // Render all meshes in the model
                for (size_t mesh_idx = 0; mesh_idx < model_to_use.meshes.size(); ++mesh_idx)
                {
                    const auto& mesh = model_to_use.meshes[mesh_idx];
                    
                    // Use texture if available (use first texture for now, or match to mesh)
                    bool has_texture = !model_to_use.textures.empty();
                    if (has_texture)
                    {
                        size_t texture_idx = (mesh_idx < model_to_use.textures.size()) ? mesh_idx : 0;
                        const auto& texture = model_to_use.textures[texture_idx];
                        
                        if (texture.id != 0)
                        {
                            glUniform1i(m_render_state.use_texture_location, 1);
                            glActiveTexture(GL_TEXTURE0);
                            glBindTexture(GL_TEXTURE_2D, texture.id);
                        }
                        else
                        {
                            has_texture = false;
                        }
                    }
                    
                    if (!has_texture)
                    {
                        // No texture, use vertex colors (which should be loaded from model)
                        glUniform1i(m_render_state.use_texture_location, 0);
                        glBindTexture(GL_TEXTURE_2D, 0);
                    }
                    
                    glBindVertexArray(mesh.vao);
                    glDrawElements(GL_TRIANGLES, mesh.index_count, GL_UNSIGNED_INT, nullptr);
                }
                
                // Reset texture state
                glUniform1i(m_render_state.use_texture_location, 0);
                glBindTexture(GL_TEXTURE_2D, 0);
            }
            else if (use_player2)
            {
                // Render player2 using GLB model (same as player1)
                const GLTFModel& model_to_use = game_state.player2_model_glb;
                
                // Scale and position - same as player1
                const float model_scale = game_state.player_radius * 2.0f;
                
                // Apply transforms - same as player1
                glm::mat4 model = glm::translate(glm::mat4(1.0f), player_position);
                model = model * glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // Flip if upside down
                model = model * glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // Stand up
                model = model * glm::scale(glm::mat4(1.0f), glm::vec3(model_scale));
                model = model * model_to_use.base_transform;
                
                // Apply animation transforms if available
                apply_animation_transform(model, game_state.player_animations[i]);
                
                const glm::mat4 mvp = projection * view * model;
                glUniformMatrix4fv(m_render_state.mvp_location, 1, GL_FALSE, glm::value_ptr(mvp));
                
                // Render all meshes in the model
                for (size_t mesh_idx = 0; mesh_idx < model_to_use.meshes.size(); ++mesh_idx)
                {
                    const auto& mesh = model_to_use.meshes[mesh_idx];
                    
                    // Use texture if available (use first texture for now, or match to mesh)
                    bool has_texture = !model_to_use.textures.empty();
                    if (has_texture)
                    {
                        size_t texture_idx = (mesh_idx < model_to_use.textures.size()) ? mesh_idx : 0;
                        const auto& texture = model_to_use.textures[texture_idx];
                        
                        if (texture.id != 0)
                        {
                            glUniform1i(m_render_state.use_texture_location, 1);
                            glActiveTexture(GL_TEXTURE0);
                            glBindTexture(GL_TEXTURE_2D, texture.id);
                        }
                        else
                        {
                            has_texture = false;
                        }
                    }
                    
                    if (!has_texture)
                    {
                        // No texture, use vertex colors (which should be loaded from model)
                        glUniform1i(m_render_state.use_texture_location, 0);
                        glBindTexture(GL_TEXTURE_2D, 0);
                    }
                    
                    glBindVertexArray(mesh.vao);
                    glDrawElements(GL_TRIANGLES, mesh.index_count, GL_UNSIGNED_INT, nullptr);
                }
                
                // Reset texture state
                glUniform1i(m_render_state.use_texture_location, 0);
                glBindTexture(GL_TEXTURE_2D, 0);
            }
            else if (use_player1)
            {
                // Render player1 using GLTF model
                const GLTFModel& model_to_use = game_state.player_model_glb;
                
                // Scale and position the player model
                const float model_scale = game_state.player_radius * 2.0f;
                
                // Apply transforms
                glm::mat4 model = glm::translate(glm::mat4(1.0f), player_position);
                model = model * glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // Flip if upside down
                model = model * glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // Stand up
                model = model * glm::scale(glm::mat4(1.0f), glm::vec3(model_scale));
                model = model * model_to_use.base_transform;
                
                // Apply animation transforms if available
                apply_animation_transform(model, game_state.player_animations[i]);
                
                const glm::mat4 mvp = projection * view * model;
                glUniformMatrix4fv(m_render_state.mvp_location, 1, GL_FALSE, glm::value_ptr(mvp));
                
                // Render all meshes in the model
                for (size_t mesh_idx = 0; mesh_idx < model_to_use.meshes.size(); ++mesh_idx)
                {
                    const auto& mesh = model_to_use.meshes[mesh_idx];
                    
                    // Use texture if available
                    bool has_texture = !model_to_use.textures.empty();
                    if (has_texture)
                    {
                        size_t texture_idx = (mesh_idx < model_to_use.textures.size()) ? mesh_idx : 0;
                        const auto& texture = model_to_use.textures[texture_idx];
                        
                        if (texture.id != 0)
                        {
                            glUniform1i(m_render_state.use_texture_location, 1);
                            glActiveTexture(GL_TEXTURE0);
                            glBindTexture(GL_TEXTURE_2D, texture.id);
                            if (m_render_state.use_color_override_location >= 0)
                            {
                                glUniform1i(m_render_state.use_color_override_location, 0);
                            }
                            has_texture = true;
                        }
                        else
                        {
                            has_texture = false;
                        }
                    }
                    
                    if (!has_texture)
                    {
                        glUniform1i(m_render_state.use_texture_location, 0);
                        glBindTexture(GL_TEXTURE_2D, 0);
                        if (m_render_state.use_color_override_location >= 0)
                        {
                            glUniform1i(m_render_state.use_color_override_location, 0);
                        }
                    }
                    
                    glBindVertexArray(mesh.vao);
                    glDrawElements(GL_TRIANGLES, mesh.index_count, GL_UNSIGNED_INT, nullptr);
                }
                
                // Reset texture state
                glUniform1i(m_render_state.use_texture_location, 0);
                glBindTexture(GL_TEXTURE_2D, 0);
                if (m_render_state.use_color_override_location >= 0)
                {
                    glUniform1i(m_render_state.use_color_override_location, 0);
                }
            }
            else
            {
                // Fallback to sphere
                const glm::mat4 model = glm::translate(glm::mat4(1.0f), player_position);
                const glm::mat4 mvp = projection * view * model;
                glUniformMatrix4fv(m_render_state.mvp_location, 1, GL_FALSE, glm::value_ptr(mvp));
                glBindVertexArray(game_state.sphere_mesh.vao);
                glDrawElements(GL_TRIANGLES, game_state.sphere_mesh.index_count, GL_UNSIGNED_INT, nullptr);
            }
        }
    }

    void Renderer::render_dice(const glm::mat4& projection, const glm::mat4& view, const GameState& game_state)
    {
        if (!(game_state.dice_state.is_rolling || game_state.dice_state.is_falling || game_state.dice_state.is_displaying))
        {
            return;
        }

        const std::vector<Mesh>* dice_meshes = nullptr;
        if (game_state.has_dice_model)
        {
            if (game_state.is_obj_format)
            {
                if (!game_state.dice_model_obj.meshes.empty())
                    dice_meshes = &game_state.dice_model_obj.meshes;
            }
            else
            {
                if (!game_state.dice_model_glb.meshes.empty())
                    dice_meshes = &game_state.dice_model_glb.meshes;
            }
        }

        if (dice_meshes && !dice_meshes->empty())
        {
            glm::vec3 render_pos = game_state.dice_state.position;
            if (game_state.dice_state.is_displaying && !game_state.dice_state.is_falling)
            {
                render_pos.y += game_state.dice_state.scale * 0.3f;
            }

            game::player::dice::DiceState temp_dice_state = game_state.dice_state;
            temp_dice_state.position = render_pos;
            glm::mat4 dice_transform = game::player::dice::get_transform(temp_dice_state);

            const glm::mat4 dice_mvp = projection * view * dice_transform;

            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(-1.0f, -1.0f);

            glUniformMatrix4fv(m_render_state.mvp_location, 1, GL_FALSE, glm::value_ptr(dice_mvp));

            if (game_state.has_dice_texture && game_state.dice_texture.id != 0)
            {
                glUniform1i(m_render_state.use_texture_location, 1);
                if (m_render_state.dice_texture_mode_location >= 0)
                {
                    glUniform1i(m_render_state.dice_texture_mode_location, 1);
                }
                glUniform1i(m_render_state.texture_location, 0);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, game_state.dice_texture.id);
            }
            else
            {
                glUniform1i(m_render_state.use_texture_location, 0);
            }

            glBindVertexArray((*dice_meshes)[0].vao);
            glDrawElements(GL_TRIANGLES, (*dice_meshes)[0].index_count, GL_UNSIGNED_INT, nullptr);

            if (game_state.has_dice_texture)
            {
                glBindTexture(GL_TEXTURE_2D, 0);
                if (m_render_state.dice_texture_mode_location >= 0)
                {
                    glUniform1i(m_render_state.dice_texture_mode_location, 0);
                }
                glUniform1i(m_render_state.use_texture_location, 0);
            }

            glDisable(GL_POLYGON_OFFSET_FILL);
        }
    }

    void Renderer::render_ui(const core::Window& window, const GameState& game_state)
    {
        const bool precision_running = game::minigame::is_running(game_state.minigame_state);
        const bool tile_memory_active = game::minigame::tile_memory::is_active(game_state.tile_memory_state);
        const bool reaction_running = game::minigame::is_running(game_state.reaction_state);
        const bool reaction_has_result = game::minigame::is_success(game_state.reaction_state) || 
                                         game::minigame::is_failure(game_state.reaction_state);
        const bool math_running = game::minigame::is_running(game_state.math_state);
        const bool math_has_result = game::minigame::is_success(game_state.math_state) || 
                                     game::minigame::is_failure(game_state.math_state);
        const bool pattern_running = game::minigame::is_running(game_state.pattern_state);
        const bool pattern_has_result = game::minigame::is_success(game_state.pattern_state) || 
                                         game::minigame::is_failure(game_state.pattern_state);
        const bool precision_showing_time = game_state.minigame_state.is_showing_time;
        const bool precision_has_result = game::minigame::is_success(game_state.minigame_state) || 
                                         game::minigame::is_failure(game_state.minigame_state);
        const auto& current_player = game_state.players[game_state.current_player_index];
        
        // Check if player can roll dice (to show "SPACE!" prompt)
        // Don't show when menu or win screen is active, or when current player is AI
        const bool can_roll_dice = !game_state.menu_state.is_active &&
                                  !game_state.win_state.is_active &&
                                  !current_player.is_ai &&  // Don't show for AI players
                                  !current_player.is_stepping && 
                                  current_player.steps_remaining == 0 && 
                                  !game_state.dice_state.is_rolling && 
                                  !game_state.dice_state.is_falling && 
                                  !game_state.dice_state.is_displaying &&
                                  !precision_running &&
                                  !tile_memory_active &&
                                  !reaction_running &&
                                  !math_running &&
                                  !pattern_running &&
                                  current_player.last_dice_result == 0;
        
        const bool show_ui_overlay = game_state.dice_state.is_displaying || 
                                    game_state.dice_state.is_rolling ||
                                    game_state.dice_state.is_falling ||
                                    current_player.steps_remaining > 0 || 
                                    precision_running ||
                                    precision_showing_time ||
                                    precision_has_result ||
                                    tile_memory_active ||
                                    reaction_running ||
                                    reaction_has_result ||
                                    math_running ||
                                    math_has_result ||
                                    pattern_running ||
                                    pattern_has_result ||
                                    game_state.debug_warp_state.active || 
                                    game_state.debug_warp_state.notification_timer > 0.0f ||
                                    game_state.minigame_message_timer > 0.0f ||
                                    can_roll_dice;  // Show UI when player can roll dice

        if (game_state.minigame_message_timer <= 0.0f && !show_ui_overlay)
        {
            return;
        }

        int window_width, window_height;
        window.get_framebuffer_size(&window_width, &window_height);

        glm::mat4 ortho_projection = glm::ortho(0.0f, static_cast<float>(window_width), 
                                                static_cast<float>(window_height), 0.0f, -1.0f, 1.0f);
        glm::mat4 identity_view = glm::mat4(1.0f);
        glm::mat4 ui_mvp = ortho_projection * identity_view;

        glUseProgram(m_render_state.program);
        glUniformMatrix4fv(m_render_state.mvp_location, 1, GL_FALSE, glm::value_ptr(ui_mvp));
        glUniform1i(m_render_state.use_texture_location, 1);
        if (m_render_state.dice_texture_mode_location >= 0)
        {
            glUniform1i(m_render_state.dice_texture_mode_location, 0);
        }
        glActiveTexture(GL_TEXTURE0);

        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        float center_x = window_width * 0.5f;
        float top_y = window_height * 0.1f;
        const float ui_primary_scale = 3.2f;
        const float ui_secondary_scale = 2.8f;
        const float ui_title_scale = 2.0f;  // Smaller scale for game titles

        // Check if minigame is showing title screen - render menu
        if (game_state.minigame_state.status == game::minigame::PrecisionTimingStatus::ShowingTitle)
        {
            game::minigame::menu::render_minigame_menu(window, &m_render_state,
                "PRECISION TIMING GAME",
                "Press SPACE to stop the timer at exactly 4.99!",
                "Get as close as possible to 4.99 seconds",
                6);
            return;
        }
        else if (game_state.tile_memory_state.phase == game::minigame::tile_memory::Phase::ShowingTitle)
        {
            game::minigame::menu::render_minigame_menu(window, &m_render_state,
                "TILE MEMORY GAME",
                "Remember the sequence of numbers shown",
                "Enter the numbers in the correct order",
                4);
            return;
        }
        else if (game_state.reaction_state.phase == game::minigame::ReactionState::Phase::ShowingTitle)
        {
            game::minigame::menu::render_minigame_menu(window, &m_render_state,
                "NUMBER GUESSING GAME",
                "Guess the number between 1 and 9",
                "You have 3 attempts to guess correctly",
                3);
            return;
        }
        else if (game_state.math_state.phase == game::minigame::MathQuizState::Phase::ShowingTitle)
        {
            game::minigame::menu::render_minigame_menu(window, &m_render_state,
                "MATH QUIZ",
                "Solve the math problem correctly",
                "Enter your answer using number keys",
                4);
            return;
        }
        else if (game_state.pattern_state.phase == game::minigame::PatternMatchingState::Phase::ShowingTitle)
        {
            game::minigame::menu::render_minigame_menu(window, &m_render_state,
                "PATTERN MATCHING",
                "Remember and repeat the pattern",
                "Use W, S, A, D keys to match the pattern",
                5);
            return;
        }

        // Priority order for UI display
        if (game_state.minigame_state.is_showing_time)
        {
            std::string time_text = game::minigame::get_display_text(game_state.minigame_state);
            glm::vec3 timing_color = {0.9f, 0.9f, 0.3f};
            // Show (space) in green when showing time
            float line_height = ui_title_scale * 70.0f;  // Increased spacing for (space) line
            const float space_scale = ui_secondary_scale * 0.8f;  // Smaller scale for (space) line
            const glm::vec3 green_color(0.2f, 1.0f, 0.4f);  // Green color for (space)
            render_text(m_render_state.text_renderer, time_text, center_x, top_y, ui_primary_scale, timing_color);
            render_text(m_render_state.text_renderer, "(space)", center_x, top_y + line_height, space_scale, green_color);
        }
        else if (!game_state.minigame_state.is_showing_time && 
                 (game::minigame::is_success(game_state.minigame_state) || 
                  game::minigame::is_failure(game_state.minigame_state)))
        {
            std::string result_text = game::minigame::get_display_text(game_state.minigame_state);
            glm::vec3 result_color = game::minigame::is_success(game_state.minigame_state) ?
                glm::vec3(0.2f, 1.0f, 0.4f) : glm::vec3(1.0f, 0.3f, 0.3f);
            render_text(m_render_state.text_renderer, result_text, center_x, top_y, ui_primary_scale, result_color);
        }
        else if (tile_memory_active)
        {
            std::string memory_text = game::minigame::tile_memory::get_display_text(game_state.tile_memory_state);
            glm::vec3 memory_color = glm::vec3(0.9f, 0.9f, 0.3f);
            bool is_result = game::minigame::tile_memory::is_result(game_state.tile_memory_state);
            if (is_result)
            {
                if (game::minigame::tile_memory::is_success(game_state.tile_memory_state))
                {
                    memory_color = glm::vec3(0.2f, 1.0f, 0.4f);
                }
                else
                {
                    memory_color = glm::vec3(1.0f, 0.3f, 0.3f);
                }
            }
            
            // Check if text contains "Bonus" - split into two lines
            size_t bonus_pos = memory_text.find("Bonus");
            if (bonus_pos != std::string::npos)
            {
                std::string game_name = memory_text.substr(0, bonus_pos);
                // Remove trailing space if exists
                while (!game_name.empty() && game_name.back() == ' ')
                {
                    game_name.pop_back();
                }
                std::string bonus_text = memory_text.substr(bonus_pos);
                float line_height = ui_title_scale * 50.0f;  // Approximate line height
                glm::vec3 game_name_color = glm::vec3(0.9f, 0.9f, 0.3f);  // Yellow color for game name
                glm::vec3 bonus_color = glm::vec3(0.2f, 1.0f, 0.4f);  // Green color for bonus
                render_text(m_render_state.text_renderer, game_name, center_x, top_y, ui_title_scale, game_name_color);
                render_text(m_render_state.text_renderer, bonus_text, center_x, top_y + line_height, ui_title_scale, bonus_color);
            }
            else if (!is_result && game_state.tile_memory_state.phase == game::minigame::tile_memory::Phase::WaitingInput)
            {
                // Show (space) in green only when waiting for input
                float line_height = ui_title_scale * 70.0f;  // Increased spacing for (space) line
                const float space_scale = ui_secondary_scale * 0.8f;  // Smaller scale for (space) line
                const glm::vec3 green_color(0.2f, 1.0f, 0.4f);  // Green color for (space)
                render_text(m_render_state.text_renderer, memory_text, center_x, top_y, ui_secondary_scale, memory_color);
                render_text(m_render_state.text_renderer, "(space)", center_x, top_y + line_height, space_scale, green_color);
            }
            else
            {
                render_text(m_render_state.text_renderer, memory_text, center_x, top_y, ui_primary_scale, memory_color);
            }
        }
        else if (game_state.debug_warp_state.active)
        {
            std::string prompt = "wrap to ";
            prompt += game_state.debug_warp_state.buffer.empty() ? "_" : game_state.debug_warp_state.buffer;
            prompt += " [enter]";
            glm::vec3 debug_color = {0.3f, 0.85f, 1.0f};
            render_text(m_render_state.text_renderer, prompt, center_x, top_y, ui_secondary_scale, debug_color);
        }
        else if (game_state.debug_warp_state.notification_timer > 0.0f && !game_state.debug_warp_state.notification.empty())
        {
            glm::vec3 debug_color = {0.3f, 0.85f, 1.0f};
            render_text(m_render_state.text_renderer, game_state.debug_warp_state.notification, 
                       center_x, top_y, ui_secondary_scale, debug_color);
        }
        else if (pattern_running || pattern_has_result)
        {
            std::string pattern_text = game::minigame::get_display_text(game_state.pattern_state);
            glm::vec3 pattern_color = {0.9f, 0.9f, 0.3f};
            bool is_result = game::minigame::is_success(game_state.pattern_state) || 
                            game::minigame::is_failure(game_state.pattern_state);
            if (game::minigame::is_success(game_state.pattern_state))
            {
                pattern_color = glm::vec3(0.2f, 1.0f, 0.4f);
            }
            else if (game::minigame::is_failure(game_state.pattern_state))
            {
                pattern_color = glm::vec3(1.0f, 0.3f, 0.3f);
            }
            
            // Check if text contains "Bonus" - split into two lines
            size_t bonus_pos = pattern_text.find("Bonus");
            if (bonus_pos != std::string::npos)
            {
                std::string game_name = pattern_text.substr(0, bonus_pos);
                // Remove trailing space if exists
                while (!game_name.empty() && game_name.back() == ' ')
                {
                    game_name.pop_back();
                }
                std::string bonus_text = pattern_text.substr(bonus_pos);
                float line_height = ui_title_scale * 50.0f;  // Approximate line height
                glm::vec3 game_name_color = glm::vec3(0.9f, 0.9f, 0.3f);  // Yellow color for game name
                glm::vec3 bonus_color = glm::vec3(0.2f, 1.0f, 0.4f);  // Green color for bonus
                render_text(m_render_state.text_renderer, game_name, center_x, top_y, ui_title_scale, game_name_color);
                render_text(m_render_state.text_renderer, bonus_text, center_x, top_y + line_height, ui_title_scale, bonus_color);
            }
            else if (!is_result && pattern_text.find("Input:") != std::string::npos)
            {
                // Show (space) in green when showing input prompt
                float line_height = ui_title_scale * 70.0f;  // Increased spacing for (space) line
                const float space_scale = ui_secondary_scale * 0.8f;  // Smaller scale for (space) line
                const glm::vec3 green_color(0.2f, 1.0f, 0.4f);  // Green color for (space)
                render_text(m_render_state.text_renderer, pattern_text, center_x, top_y, ui_secondary_scale, pattern_color);
                render_text(m_render_state.text_renderer, "(space)", center_x, top_y + line_height, space_scale, green_color);
            }
            else
            {
                render_text(m_render_state.text_renderer, pattern_text, center_x, top_y, ui_secondary_scale, pattern_color);
            }
        }
        else if (game_state.minigame_message_timer > 0.0f && !game_state.minigame_message.empty())
        {
            std::string display_msg = game_state.minigame_message;
            glm::vec3 msg_color;
            if (display_msg.find("โบนัส") != std::string::npos || 
                display_msg.find("+6") != std::string::npos ||
                display_msg.find("Bonus") != std::string::npos ||
                display_msg.find("โบน") != std::string::npos)
            {
                msg_color = glm::vec3(0.2f, 1.0f, 0.4f);
            }
            else
            {
                msg_color = glm::vec3(1.0f, 0.3f, 0.3f);
            }
            render_text(m_render_state.text_renderer, display_msg, center_x, top_y, ui_primary_scale, msg_color);
        }
        else if (reaction_running || reaction_has_result)
        {
            std::string reaction_text = game::minigame::get_display_text(game_state.reaction_state);
            
            // Skip displaying if text contains "Bonus" (title screen text)
            size_t bonus_pos = reaction_text.find("Bonus");
            if (bonus_pos != std::string::npos)
            {
                // Don't display title screen text
                return;
            }
            
            glm::vec3 reaction_color = {0.9f, 0.9f, 0.3f};
            if (game::minigame::is_success(game_state.reaction_state))
            {
                reaction_color = glm::vec3(0.2f, 1.0f, 0.4f);
            }
            else if (game::minigame::is_failure(game_state.reaction_state))
            {
                reaction_color = glm::vec3(1.0f, 0.3f, 0.3f);
            }
            
            // Check if text contains "\n" - split into two lines
            size_t newline_pos = reaction_text.find("\n");
            if (newline_pos != std::string::npos)
            {
                std::string input_line = reaction_text.substr(0, newline_pos);
                std::string space_line = reaction_text.substr(newline_pos + 1);
                float line_height = ui_title_scale * 70.0f;  // Increased spacing for (space) line
                const float space_scale = ui_secondary_scale * 0.8f;  // Smaller scale for (space) line
                const glm::vec3 green_color(0.2f, 1.0f, 0.4f);  // Green color for (space)
                render_text(m_render_state.text_renderer, input_line, center_x, top_y, ui_secondary_scale, reaction_color);
                render_text(m_render_state.text_renderer, space_line, center_x, top_y + line_height, space_scale, green_color);
            }
            else
            {
                render_text(m_render_state.text_renderer, reaction_text, center_x, top_y, ui_secondary_scale, reaction_color);
            }
        }
        else if (math_running || math_has_result)
        {
            std::string math_text = game::minigame::get_display_text(game_state.math_state);
            glm::vec3 math_color = {0.9f, 0.9f, 0.3f};
            bool is_result = game::minigame::is_success(game_state.math_state) || 
                            game::minigame::is_failure(game_state.math_state);
            if (game::minigame::is_success(game_state.math_state))
            {
                math_color = glm::vec3(0.2f, 1.0f, 0.4f);
            }
            else if (game::minigame::is_failure(game_state.math_state))
            {
                math_color = glm::vec3(1.0f, 0.3f, 0.3f);
            }
            
            // Check if text contains "Bonus" - split into two lines
            size_t bonus_pos = math_text.find("Bonus");
            if (bonus_pos != std::string::npos)
            {
                std::string game_name = math_text.substr(0, bonus_pos);
                // Remove trailing space if exists
                while (!game_name.empty() && game_name.back() == ' ')
                {
                    game_name.pop_back();
                }
                std::string bonus_text = math_text.substr(bonus_pos);
                float line_height = ui_title_scale * 50.0f;  // Approximate line height
                glm::vec3 game_name_color = glm::vec3(0.9f, 0.9f, 0.3f);  // Yellow color for game name
                glm::vec3 bonus_color = glm::vec3(0.2f, 1.0f, 0.4f);  // Green color for bonus
                render_text(m_render_state.text_renderer, game_name, center_x, top_y, ui_title_scale, game_name_color);
                render_text(m_render_state.text_renderer, bonus_text, center_x, top_y + line_height, ui_title_scale, bonus_color);
            }
            else if (!is_result && math_text.find("=") != std::string::npos)
            {
                // Show (space) in green when showing question (not result)
                float line_height = ui_title_scale * 70.0f;  // Increased spacing for (space) line
                const float space_scale = ui_secondary_scale * 0.8f;  // Smaller scale for (space) line
                const glm::vec3 green_color(0.2f, 1.0f, 0.4f);  // Green color for (space)
                render_text(m_render_state.text_renderer, math_text, center_x, top_y, ui_secondary_scale, math_color);
                render_text(m_render_state.text_renderer, "(space)", center_x, top_y + line_height, space_scale, green_color);
            }
            else
            {
                render_text(m_render_state.text_renderer, math_text, center_x, top_y, ui_secondary_scale, math_color);
            }
        }
        else if (precision_running)
        {
            std::string timing_text = game::minigame::get_display_text(game_state.minigame_state);
            glm::vec3 timing_color = {0.9f, 0.9f, 0.3f};
            bool is_result = game::minigame::is_success(game_state.minigame_state) || 
                            game::minigame::is_failure(game_state.minigame_state);
            
            // Check if text contains "Bonus" - split into two lines
            size_t bonus_pos = timing_text.find("Bonus");
            if (bonus_pos != std::string::npos)
            {
                std::string game_name = timing_text.substr(0, bonus_pos);
                // Remove trailing space if exists
                while (!game_name.empty() && game_name.back() == ' ')
                {
                    game_name.pop_back();
                }
                std::string bonus_text = timing_text.substr(bonus_pos);
                float line_height = ui_title_scale * 50.0f;  // Approximate line height
                glm::vec3 game_name_color = glm::vec3(0.9f, 0.9f, 0.3f);  // Yellow color for game name
                glm::vec3 bonus_color = glm::vec3(0.2f, 1.0f, 0.4f);  // Green color for bonus
                render_text(m_render_state.text_renderer, game_name, center_x, top_y, ui_title_scale, game_name_color);
                render_text(m_render_state.text_renderer, bonus_text, center_x, top_y + line_height, ui_title_scale, bonus_color);
            }
            else if (!is_result && !game_state.minigame_state.is_showing_time && timing_text.find("4.99:") != std::string::npos)
            {
                // Show (space) in green when running (showing timer, not result)
                float line_height = ui_title_scale * 70.0f;  // Increased spacing for (space) line
                const float space_scale = ui_secondary_scale * 0.8f;  // Smaller scale for (space) line
                const glm::vec3 green_color(0.2f, 1.0f, 0.4f);  // Green color for (space)
                render_text(m_render_state.text_renderer, timing_text, center_x, top_y, ui_secondary_scale, timing_color);
                render_text(m_render_state.text_renderer, "(space)", center_x, top_y + line_height, space_scale, green_color);
            }
            else
            {
                render_text(m_render_state.text_renderer, timing_text, center_x, top_y, ui_secondary_scale, timing_color);
            }
        }
        else if (game_state.dice_display_timer > 0.0f && game_state.dice_state.result > 0)
        {
            std::string dice_text = std::to_string(game_state.dice_state.result);
            glm::vec3 text_color(1.0f, 1.0f, 0.0f);
            render_text(m_render_state.text_renderer, dice_text, center_x, top_y, ui_primary_scale, text_color);
            
            // Show current player info if multiple players
            if (game_state.num_players > 1)
            {
                std::ostringstream player_info;
                player_info << "Player " << (game_state.current_player_index + 1) << "/" << game_state.num_players;
                const float player_info_scale = ui_secondary_scale * 0.6f;
                const float player_info_y = top_y + ui_title_scale * 80.0f;
                const glm::vec3 player_info_color(0.7f, 0.7f, 1.0f);  // Light blue
                render_text(m_render_state.text_renderer, player_info.str(), center_x, player_info_y, player_info_scale, player_info_color);
            }
        }
        else if (can_roll_dice)
        {
            // Show "SPACE!" in green when player can roll dice
            const float space_scale = ui_primary_scale * 1.2f;  // Larger scale for "SPACE!"
            const glm::vec3 green_color(0.2f, 1.0f, 0.4f);  // Green color
            render_text(m_render_state.text_renderer, "SPACE!", center_x, top_y, space_scale, green_color);
        }

        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glUniform1i(m_render_state.use_texture_location, 0);
        if (m_render_state.dice_texture_mode_location >= 0)
        {
            glUniform1i(m_render_state.dice_texture_mode_location, 0);
        }
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

