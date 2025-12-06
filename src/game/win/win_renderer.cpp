#include "win_renderer.h"
#include "../game_state.h"

#include "../../rendering/text_renderer.h"
#include "../../core/window.h"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <cmath>
#include <vector>
#include <string>

namespace game
{
    namespace win
    {
        // Helper to render a colored quad (no texture)
        void render_colored_quad(const RenderState& render_state, const glm::mat4& mvp,
                                 float x, float y, float width, float height,
                                 float r, float g, float b, float a)
        {
            glUniformMatrix4fv(render_state.mvp_location, 1, GL_FALSE, glm::value_ptr(mvp));
            glUniform1i(render_state.use_texture_location, 0);

            const float vertices[] = {
                x,         y,          0.0f, r, g, b, a, 0.0f, 0.0f,
                x,         y + height, 0.0f, r, g, b, a, 0.0f, 0.0f,
                x + width, y + height, 0.0f, r, g, b, a, 0.0f, 0.0f,
                x,         y,          0.0f, r, g, b, a, 0.0f, 0.0f,
                x + width, y + height, 0.0f, r, g, b, a, 0.0f, 0.0f,
                x + width, y,          0.0f, r, g, b, a, 0.0f, 0.0f
            };

            static GLuint quad_vao = 0, quad_vbo = 0;
            if (quad_vao == 0)
            {
                glGenVertexArrays(1, &quad_vao);
                glGenBuffers(1, &quad_vbo);
                glBindVertexArray(quad_vao);
                glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
                glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
                glEnableVertexAttribArray(0);
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
                glEnableVertexAttribArray(1);
                glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
                glEnableVertexAttribArray(2);
                glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(7 * sizeof(float)));
            }

            glBindVertexArray(quad_vao);
            glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glBindVertexArray(0);
        }

        // Helper to render a circle
        void render_circle(const RenderState& render_state, const glm::mat4& mvp,
                          float center_x, float center_y, float radius,
                          float r, float g, float b, float a)
        {
            glUniformMatrix4fv(render_state.mvp_location, 1, GL_FALSE, glm::value_ptr(mvp));
            glUniform1i(render_state.use_texture_location, 0);

            const int segments = 32;
            const float angle_step = 2.0f * 3.14159265f / segments;

            std::vector<float> vertices;
            vertices.reserve(segments * 3 * 9);

            for (int i = 0; i < segments; i++)
            {
                float angle1 = i * angle_step;
                float angle2 = (i + 1) * angle_step;

                vertices.push_back(center_x);
                vertices.push_back(center_y);
                vertices.push_back(0.0f);
                vertices.push_back(r);
                vertices.push_back(g);
                vertices.push_back(b);
                vertices.push_back(a);
                vertices.push_back(0.0f);
                vertices.push_back(0.0f);

                vertices.push_back(center_x + radius * cosf(angle1));
                vertices.push_back(center_y + radius * sinf(angle1));
                vertices.push_back(0.0f);
                vertices.push_back(r);
                vertices.push_back(g);
                vertices.push_back(b);
                vertices.push_back(a);
                vertices.push_back(0.0f);
                vertices.push_back(0.0f);

                vertices.push_back(center_x + radius * cosf(angle2));
                vertices.push_back(center_y + radius * sinf(angle2));
                vertices.push_back(0.0f);
                vertices.push_back(r);
                vertices.push_back(g);
                vertices.push_back(b);
                vertices.push_back(a);
                vertices.push_back(0.0f);
                vertices.push_back(0.0f);
            }

            static GLuint vao_circle = 0, vbo_circle = 0;
            if (vao_circle == 0)
            {
                glGenVertexArrays(1, &vao_circle);
                glGenBuffers(1, &vbo_circle);
            }

            glBindVertexArray(vao_circle);
            glBindBuffer(GL_ARRAY_BUFFER, vbo_circle);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);

            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(7 * sizeof(float)));

            glDrawArrays(GL_TRIANGLES, 0, segments * 3);
            glBindVertexArray(0);
        }

        void render_win_screen(const core::Window& window, const void* render_state_ptr,
                              const WinState& win_state)
        {
            if (!render_state_ptr || !win_state.is_active)
            {
                return;
            }

            const RenderState& render_state = *static_cast<const RenderState*>(render_state_ptr);

            int window_width, window_height;
            window.get_framebuffer_size(&window_width, &window_height);

            // Setup orthographic projection for UI
            glm::mat4 ortho_projection = glm::ortho(0.0f, static_cast<float>(window_width),
                                                    static_cast<float>(window_height), 0.0f, -1.0f, 1.0f);
            glm::mat4 identity_view = glm::mat4(1.0f);
            glm::mat4 ui_mvp = ortho_projection * identity_view;

            glUseProgram(render_state.program);
            if (render_state.dice_texture_mode_location >= 0)
            {
                glUniform1i(render_state.dice_texture_mode_location, 0);
            }

            // Enable blending for transparency
            glDisable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glBlendEquation(GL_FUNC_ADD);

            // Render semi-transparent overlay
            const float overlay_alpha = 0.85f;
            render_colored_quad(render_state, ui_mvp, 0.0f, 0.0f, 
                               static_cast<float>(window_width), static_cast<float>(window_height),
                               0.0f, 0.0f, 0.0f, overlay_alpha);

            // Render win panel (similar to menu panel)
            const float popup_width = static_cast<float>(window_width) * 0.6f;
            const float popup_height = static_cast<float>(window_height) * 0.5f;
            const float popup_x = (static_cast<float>(window_width) - popup_width) * 0.5f;
            const float popup_y = (static_cast<float>(window_height) - popup_height) * 0.5f;

            // Panel colors
            const float panel_body_r = 39.0f / 255.0f;
            const float panel_body_g = 35.0f / 255.0f;
            const float panel_body_b = 75.0f / 255.0f;
            const float panel_alpha = 1.0f;

            const float panel_top_r = 26.0f / 255.0f;
            const float panel_top_g = 24.0f / 255.0f;
            const float panel_top_b = 54.0f / 255.0f;

            // Render main panel body
            render_colored_quad(render_state, ui_mvp, popup_x, popup_y, popup_width, popup_height,
                               panel_body_r, panel_body_g, panel_body_b, panel_alpha);

            // Render header bar
            const float header_height = 30.0f;
            render_colored_quad(render_state, ui_mvp, popup_x, popup_y, popup_width, header_height,
                               panel_top_r, panel_top_g, panel_top_b, panel_alpha);

            // Render bottom bar
            const float bottom_height = 10.0f;
            render_colored_quad(render_state, ui_mvp, popup_x, popup_y + popup_height - bottom_height,
                               popup_width, bottom_height,
                               panel_top_r, panel_top_g, panel_top_b, panel_alpha);

            // Render window control buttons
            const float button_size = 12.0f;
            const float button_spacing = 4.0f;
            const float button_y = popup_y + 8.0f;
            float button_x = popup_x + 8.0f;

            // Red button
            render_circle(render_state, ui_mvp, button_x + button_size * 0.5f, button_y + button_size * 0.5f,
                         button_size * 0.5f, 225.0f/255.0f, 86.0f/255.0f, 89.0f/255.0f, panel_alpha);

            // Orange button
            button_x += button_size + button_spacing;
            render_circle(render_state, ui_mvp, button_x + button_size * 0.5f, button_y + button_size * 0.5f,
                         button_size * 0.5f, 223.0f/255.0f, 163.0f/255.0f, 40.0f/255.0f, panel_alpha);

            // Green button
            button_x += button_size + button_spacing;
            render_circle(render_state, ui_mvp, button_x + button_size * 0.5f, button_y + button_size * 0.5f,
                         button_size * 0.5f, 43.0f/255.0f, 198.0f/255.0f, 66.0f/255.0f, panel_alpha);

            // Render win text
            glUniformMatrix4fv(render_state.mvp_location, 1, GL_FALSE, glm::value_ptr(ui_mvp));
            glUniform1i(render_state.use_texture_location, 1);
            if (render_state.dice_texture_mode_location >= 0)
            {
                glUniform1i(render_state.dice_texture_mode_location, 0);
            }

            const float title_scale = 2.2f;
            const float title_x = popup_x + popup_width * 0.5f;
            const float title_y = popup_y + header_height + 80.0f;
            const glm::vec3 win_color(238.0f/255.0f, 213.0f/255.0f, 18.0f/255.0f); // Yellow

            // Animated scale for "YOU WIN!" text
            float scale_factor = 1.0f;
            if (win_state.show_animation)
            {
                // Pulse animation
                scale_factor = 1.0f + 0.1f * sinf(win_state.animation_timer * 3.0f);
            }

            render_text(render_state.text_renderer, "YOU WIN!", title_x, title_y, title_scale * scale_factor, win_color);

            // Render player number
            const float player_scale = 1.5f;
            const float player_y = title_y + 100.0f;
            const glm::vec3 white_color(1.0f, 1.0f, 1.0f);
            std::string player_text = "Player " + std::to_string(win_state.winner_player) + " Wins!";
            render_text(render_state.text_renderer, player_text, title_x, player_y, player_scale, white_color);

            // Render instruction text
            const float instruction_scale = 0.9f;
            const float instruction_y = popup_y + popup_height - 80.0f;
            const glm::vec3 light_purple_color(151.0f/255.0f, 134.0f/255.0f, 215.0f/255.0f);
            render_text(render_state.text_renderer, "Press Space to Return to Menu", title_x, instruction_y, instruction_scale, light_purple_color);

            // Render confetti effect (simple circles)
            if (win_state.show_animation)
            {
                const int confetti_count = 20;
                const float confetti_radius = 5.0f;
                for (int i = 0; i < confetti_count; i++)
                {
                    float angle = (win_state.animation_timer * 2.0f + i * 0.3f) * 3.14159f;
                    float radius_offset = 100.0f + 50.0f * sinf(win_state.animation_timer + i);
                    float confetti_x = title_x + radius_offset * cosf(angle);
                    float confetti_y = title_y + radius_offset * sinf(angle);
                    
                    // Random colors for confetti
                    float r = 0.5f + 0.5f * sinf(i * 0.7f);
                    float g = 0.5f + 0.5f * sinf(i * 0.9f);
                    float b = 0.5f + 0.5f * sinf(i * 1.1f);
                    
                    render_circle(render_state, ui_mvp, confetti_x, confetti_y, confetti_radius,
                                r, g, b, 0.8f);
                }
            }

            glDisable(GL_BLEND);
            glEnable(GL_DEPTH_TEST);
            glUniform1i(render_state.use_texture_location, 0);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }
}

