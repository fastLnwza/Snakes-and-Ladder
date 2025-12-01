#include "ui_overlay.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "core/window.h"
#include "game/minigame/qte_minigame.h"
#include "game/minigame/tile_memory_minigame.h"
#include "game/player/dice/dice.h"
#include "game/debug/debug_warp_state.h"
#include "rendering/text/text_renderer.h"

namespace game::ui
{
    void render_ui_overlay(const core::Window& window,
                           GLuint program,
                           GLint mvp_location,
                           GLint use_texture_location,
                           GLint dice_texture_mode_location,
                           TextRenderer& text_renderer,
                           const game::player::dice::DiceState& dice_state,
                           const game::minigame::PrecisionTimingState& precision_state,
                           const game::minigame::tile_memory::TileMemoryState& tile_memory_state,
                           const DebugWarpState& debug_warp_state,
                           float minigame_message_timer,
                           const std::string& minigame_message,
                           float dice_display_timer,
                           int player_steps_remaining)
    {
        const bool show_ui_overlay = dice_state.is_displaying || dice_state.is_rolling || dice_state.is_falling ||
                                     player_steps_remaining > 0 || game::minigame::is_running(precision_state) ||
                                     precision_state.is_showing_time ||
                                     game::minigame::is_success(precision_state) ||
                                     game::minigame::is_failure(precision_state) ||
                                     game::minigame::tile_memory::is_active(tile_memory_state) ||
                                     debug_warp_state.active || debug_warp_state.notification_timer > 0.0f ||
                                     minigame_message_timer > 0.0f;

        if (!(minigame_message_timer > 0.0f || show_ui_overlay))
        {
            return;
        }

        int window_width = 0;
        int window_height = 0;
        window.get_framebuffer_size(&window_width, &window_height);

        glm::mat4 ortho_projection = glm::ortho(0.0f, static_cast<float>(window_width),
                                                static_cast<float>(window_height), 0.0f, -1.0f, 1.0f);
        glm::mat4 identity_view = glm::mat4(1.0f);
        glm::mat4 ui_mvp = ortho_projection * identity_view;

        glUseProgram(program);
        glUniformMatrix4fv(mvp_location, 1, GL_FALSE, glm::value_ptr(ui_mvp));
        glUniform1i(use_texture_location, 1);
        if (dice_texture_mode_location >= 0)
        {
            glUniform1i(dice_texture_mode_location, 0);
        }
        glActiveTexture(GL_TEXTURE0);

        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        const float center_x = window_width * 0.5f;
        const float top_y = window_height * 0.1f;
        constexpr float ui_primary_scale = 3.2f;
        constexpr float ui_secondary_scale = 2.8f;

        if (precision_state.is_showing_time)
        {
            std::string time_text = game::minigame::get_display_text(precision_state);
            const glm::vec3 timing_color = {0.9f, 0.9f, 0.3f};
            render_text(text_renderer, time_text, center_x, top_y, ui_primary_scale, timing_color);
        }
        else if (!precision_state.is_showing_time &&
                 (game::minigame::is_success(precision_state) || game::minigame::is_failure(precision_state)))
        {
            std::string result_text = game::minigame::get_display_text(precision_state);
            const glm::vec3 result_color = game::minigame::is_success(precision_state)
                                               ? glm::vec3(0.2f, 1.0f, 0.4f)
                                               : glm::vec3(1.0f, 0.3f, 0.3f);
            render_text(text_renderer, result_text, center_x, top_y, ui_primary_scale, result_color);
        }
        else if (game::minigame::is_running(precision_state))
        {
            std::string running_text = game::minigame::get_display_text(precision_state);
            const glm::vec3 timing_color = {0.9f, 0.9f, 0.3f};
            render_text(text_renderer, running_text, center_x, top_y, ui_primary_scale, timing_color);
        }
        else if (game::minigame::tile_memory::is_active(tile_memory_state))
        {
            std::string memory_text = game::minigame::tile_memory::get_display_text(tile_memory_state);
            glm::vec3 memory_color;
            if (game::minigame::tile_memory::is_result(tile_memory_state))
            {
                memory_color = game::minigame::tile_memory::is_success(tile_memory_state)
                                   ? glm::vec3(0.2f, 1.0f, 0.4f)
                                   : glm::vec3(1.0f, 0.3f, 0.3f);
            }
            else
            {
                memory_color = glm::vec3(0.9f, 0.9f, 0.3f);
            }
            render_text(text_renderer, memory_text, center_x, top_y, ui_primary_scale, memory_color);
        }
        else if (debug_warp_state.active)
        {
            std::string prompt = "Warped to ";
            prompt += debug_warp_state.buffer.empty() ? "_" : debug_warp_state.buffer;
            prompt += "  [Enter]";
            const glm::vec3 debug_color = {0.3f, 0.85f, 1.0f};
            render_text(text_renderer, prompt, center_x, top_y, ui_secondary_scale, debug_color);
        }
        else if (debug_warp_state.notification_timer > 0.0f && !debug_warp_state.notification.empty())
        {
            const glm::vec3 debug_color = {0.3f, 0.85f, 1.0f};
            render_text(text_renderer,
                        debug_warp_state.notification,
                        center_x,
                        top_y,
                        ui_secondary_scale,
                        debug_color);
        }
        else if (minigame_message_timer > 0.0f && !minigame_message.empty())
        {
            const bool success_like = minigame_message.find("โบนัส") != std::string::npos ||
                                      minigame_message.find("+6") != std::string::npos ||
                                      minigame_message.find("Bonus") != std::string::npos ||
                                      minigame_message.find("โบน") != std::string::npos;
            const glm::vec3 msg_color = success_like ? glm::vec3(0.2f, 1.0f, 0.4f) : glm::vec3(1.0f, 0.3f, 0.3f);
            render_text(text_renderer, minigame_message, center_x, top_y, ui_primary_scale, msg_color);
        }
        else if (dice_display_timer > 0.0f && dice_state.result > 0)
        {
            const glm::vec3 text_color = {1.0f, 1.0f, 0.0f};
            render_text(text_renderer, std::to_string(dice_state.result), center_x, top_y, ui_primary_scale, text_color);
        }

        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glUniform1i(use_texture_location, 0);
        if (dice_texture_mode_location >= 0)
        {
            glUniform1i(dice_texture_mode_location, 0);
        }
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

