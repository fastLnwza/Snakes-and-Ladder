#include "game_renderer.h"

#include "game/player/player.h"
#include "game/player/dice/dice.h"
#include "game/ui/ui_overlay.h"
#include "game/map/board.h"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

using namespace game::player::dice;

namespace game
{
    void render_game(GameState& state)
    {
        // Clear screen
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Setup camera
        const float aspect_ratio = state.window->get_aspect_ratio();
        const glm::mat4 projection = state.camera.get_projection(aspect_ratio);
        const glm::vec3 player_position = get_position(state.player_state);
        const glm::mat4 view = state.camera.get_view(player_position, state.map_length);

        glUseProgram(state.shader_program);
        
        // Disable texture for map and player
        glUniform1i(state.use_texture_location, 0);
        glBindTexture(GL_TEXTURE_2D, 0);

        // Draw map
        {
            const glm::mat4 model(1.0f);
            const glm::mat4 mvp = projection * view * model;
            glUniformMatrix4fv(state.mvp_location, 1, GL_FALSE, glm::value_ptr(mvp));
            glBindVertexArray(state.map_mesh.vao);
            glDrawElements(GL_TRIANGLES, state.map_mesh.index_count, GL_UNSIGNED_INT, nullptr);
        }

        // Draw player
        {
            const glm::mat4 model = glm::translate(glm::mat4(1.0f), player_position);
            const glm::mat4 mvp = projection * view * model;
            glUniformMatrix4fv(state.mvp_location, 1, GL_FALSE, glm::value_ptr(mvp));
            glBindVertexArray(state.player_mesh.vao);
            glDrawElements(GL_TRIANGLES, state.player_mesh.index_count, GL_UNSIGNED_INT, nullptr);
        }

        // Draw dice when rolling, falling, or displaying result
        if (state.dice_state.is_rolling || state.dice_state.is_falling || state.dice_state.is_displaying)
        {
            const std::vector<Mesh>* dice_meshes = nullptr;
            if (state.has_dice_model)
            {
                if (state.is_obj_format)
                {
                    if (!state.dice_model_obj.meshes.empty())
                        dice_meshes = &state.dice_model_obj.meshes;
                }
                else
                {
                    if (!state.dice_model_glb.meshes.empty())
                        dice_meshes = &state.dice_model_glb.meshes;
                }
            }
            
            if (dice_meshes && !dice_meshes->empty())
            {
                // Use dice position with offset to prevent sinking
                glm::vec3 render_pos = state.dice_state.position;
                if (state.dice_state.is_displaying && !state.dice_state.is_falling)
                {
                    render_pos.y += state.dice_state.scale * 0.3f;
                }
                
                // Create transform with adjusted position
                DiceState temp_dice_state = state.dice_state;
                temp_dice_state.position = render_pos;
                glm::mat4 dice_transform = get_transform(temp_dice_state);
                
                const glm::mat4 dice_mvp = projection * view * dice_transform;
                
                // Enable polygon offset to make dice render in front
                glEnable(GL_POLYGON_OFFSET_FILL);
                glPolygonOffset(-1.0f, -1.0f);
                
                glUniformMatrix4fv(state.mvp_location, 1, GL_FALSE, glm::value_ptr(dice_mvp));
            
                // Enable texture if available
                if (state.has_dice_texture && state.dice_texture.id != 0)
                {
                    glUseProgram(state.shader_program);
                    glUniform1i(state.use_texture_location, 1);
                    if (state.dice_texture_mode_location >= 0)
                    {
                        glUniform1i(state.dice_texture_mode_location, 1);
                    }
                    glUniform1i(state.texture_location, 0);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, state.dice_texture.id);
                }
                else
                {
                    glUniform1i(state.use_texture_location, 0);
                }
                
                // Draw dice mesh
                if (!dice_meshes->empty())
                {
                    glBindVertexArray((*dice_meshes)[0].vao);
                    glDrawElements(GL_TRIANGLES, (*dice_meshes)[0].index_count, GL_UNSIGNED_INT, nullptr);
                }
                
                // Unbind texture
                if (state.has_dice_texture)
                {
                    glBindTexture(GL_TEXTURE_2D, 0);
                    if (state.dice_texture_mode_location >= 0)
                    {
                        glUniform1i(state.dice_texture_mode_location, 0);
                    }
                    glUniform1i(state.use_texture_location, 0);
                }
                
                // Disable polygon offset
                glDisable(GL_POLYGON_OFFSET_FILL);
            }
        }

        // Render UI overlay
        game::ui::render_ui_overlay(*state.window,
                                    state.shader_program,
                                    state.mvp_location,
                                    state.use_texture_location,
                                    state.dice_texture_mode_location,
                                    state.text_renderer,
                                    state.dice_state,
                                    state.minigame_state,
                                    state.tile_memory_state,
                                    state.debug_warp_state,
                                    state.minigame_message_timer,
                                    state.minigame_message,
                                    state.dice_display_timer,
                                    state.player_state.steps_remaining);

        state.window->swap_buffers();
    }
}

