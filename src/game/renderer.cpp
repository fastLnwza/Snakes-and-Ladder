#include "renderer.h"

#include "../game/map/map_manager.h"
#include "../game/player/player.h"
#include "../game/player/dice/dice.h"
#include "../game/minigame/qte_minigame.h"
#include "../game/minigame/tile_memory_minigame.h"
#include "../rendering/text_renderer.h"
#include "../rendering/mesh.h"

#include <glad/glad.h>
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
        const glm::vec3 player_position = get_position(game_state.player_state);
        const glm::mat4 view = camera.get_view(player_position, game_state.map_length);

        glUseProgram(m_render_state.program);
        glUniform1i(m_render_state.use_texture_location, 0);
        glBindTexture(GL_TEXTURE_2D, 0);

        render_map(projection, view, game_state);
        render_player(projection, view, game_state);
        render_dice(projection, view, game_state);
        render_ui(window, game_state);
    }

    void Renderer::render_map(const glm::mat4& projection, const glm::mat4& view, const GameState& game_state)
    {
        game::map::render_map(game_state.map_data, projection, view, m_render_state.program, m_render_state.mvp_location);
    }

    void Renderer::render_player(const glm::mat4& projection, const glm::mat4& view, const GameState& game_state)
    {
        const glm::vec3 player_position = get_position(game_state.player_state);
        const glm::mat4 model = glm::translate(glm::mat4(1.0f), player_position);
        const glm::mat4 mvp = projection * view * model;
        glUniformMatrix4fv(m_render_state.mvp_location, 1, GL_FALSE, glm::value_ptr(mvp));
        glBindVertexArray(game_state.sphere_mesh.vao);
        glDrawElements(GL_TRIANGLES, game_state.sphere_mesh.index_count, GL_UNSIGNED_INT, nullptr);
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
        const bool show_ui_overlay = game_state.dice_state.is_displaying || 
                                    game_state.dice_state.is_rolling || 
                                    game_state.dice_state.is_falling ||
                                    game_state.player_state.steps_remaining > 0 || 
                                    precision_running ||
                                    tile_memory_active ||
                                    game_state.debug_warp_state.active || 
                                    game_state.debug_warp_state.notification_timer > 0.0f ||
                                    game_state.minigame_message_timer > 0.0f;

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

        // Priority order for UI display
        if (game_state.minigame_state.is_showing_time)
        {
            std::string time_text = game::minigame::get_display_text(game_state.minigame_state);
            glm::vec3 timing_color = {0.9f, 0.9f, 0.3f};
            render_text(m_render_state.text_renderer, time_text, center_x, top_y, ui_primary_scale, timing_color);
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
            if (game::minigame::tile_memory::is_result(game_state.tile_memory_state))
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
            render_text(m_render_state.text_renderer, memory_text, center_x, top_y, ui_primary_scale, memory_color);
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
        else if (precision_running)
        {
            std::string timing_text = game::minigame::get_display_text(game_state.minigame_state);
            glm::vec3 timing_color = {0.9f, 0.9f, 0.3f};
            render_text(m_render_state.text_renderer, timing_text, center_x, top_y, ui_secondary_scale, timing_color);
        }
        else if (game_state.dice_display_timer > 0.0f && game_state.dice_state.result > 0)
        {
            std::string dice_text = std::to_string(game_state.dice_state.result);
            glm::vec3 text_color(1.0f, 1.0f, 0.0f);
            render_text(m_render_state.text_renderer, dice_text, center_x, top_y, ui_primary_scale, text_color);
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

