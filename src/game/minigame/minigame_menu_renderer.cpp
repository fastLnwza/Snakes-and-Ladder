#include "minigame_menu_renderer.h"
#include "../game_state.h"

#include "../../rendering/text_renderer.h"
#include "../../core/window.h"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <cmath>

namespace game
{
    namespace minigame
    {
        namespace menu
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

            void render_minigame_menu(const core::Window& window, const void* render_state_ptr,
                                     const std::string& game_title, const std::string& game_description,
                                     const std::string& instruction_text, int bonus_steps)
            {
                if (!render_state_ptr)
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

                // Render Panel as popup in center of screen
                const float popup_width = static_cast<float>(window_width) * 0.5f;
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

                // Render text
                glUniformMatrix4fv(render_state.mvp_location, 1, GL_FALSE, glm::value_ptr(ui_mvp));
                glUniform1i(render_state.use_texture_location, 1);
                if (render_state.dice_texture_mode_location >= 0)
                {
                    glUniform1i(render_state.dice_texture_mode_location, 0);
                }

                const float title_scale = 1.5f;
                const float title_x = popup_x + popup_width * 0.5f;
                const float title_y = popup_y + header_height + 50.0f;
                const glm::vec3 white_color(1.0f, 1.0f, 1.0f);
                const glm::vec3 light_purple_color(151.0f/255.0f, 134.0f/255.0f, 215.0f/255.0f);
                const glm::vec3 yellow_color(238.0f/255.0f, 213.0f/255.0f, 18.0f/255.0f);

                // Render game title
                render_text(render_state.text_renderer, game_title, title_x, title_y, title_scale, white_color);
                
                // Render description (smaller)
                const float desc_scale = 0.7f;
                const float desc_y = title_y + 90.0f; // Increased spacing from title
                render_text(render_state.text_renderer, game_description, title_x, desc_y, desc_scale, light_purple_color);
                
                // Render instruction (smaller)
                const float inst_scale = 0.65f;
                const float inst_y = desc_y + 50.0f;
                render_text(render_state.text_renderer, instruction_text, title_x, inst_y, inst_scale, light_purple_color);
                
                // Render bonus (green color)
                const float bonus_scale = 1.0f;
                const float bonus_y = inst_y + 60.0f;
                std::string bonus_text = "Bonus: +" + std::to_string(bonus_steps) + " steps";
                const glm::vec3 green_color(0.2f, 1.0f, 0.4f); // Green color for bonus
                render_text(render_state.text_renderer, bonus_text, title_x, bonus_y, bonus_scale, green_color);
                
                // Render "Press Space to Start" (yellow color)
                const float start_scale = 0.9f;
                const float start_y = popup_y + popup_height - 60.0f;
                render_text(render_state.text_renderer, "Press Space to Start", title_x, start_y, start_scale, yellow_color);

                glDisable(GL_BLEND);
                glEnable(GL_DEPTH_TEST);
                glUniform1i(render_state.use_texture_location, 0);
                glBindTexture(GL_TEXTURE_2D, 0);
            }
        }
    }
}

