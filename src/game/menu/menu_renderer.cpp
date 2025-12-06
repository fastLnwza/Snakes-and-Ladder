#include "menu_renderer.h"
#include "../game_state.h"

#include "../../rendering/texture_loader.h"
#include "../../rendering/text_renderer.h"
#include "../../core/window.h"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <cmath>

namespace game
{
    namespace menu
    {
        // Global menu textures storage
        static MenuTextures g_menu_textures{};

        bool load_menu_textures(const std::filesystem::path& assets_dir)
        {
            try
            {
                const auto ui_dir = assets_dir / "UI" / "ModernPurpleUI";
                g_menu_textures.panel = load_texture(ui_dir / "Panel.png");
                g_menu_textures.loaded = true;
                std::cout << "Menu textures loaded successfully. Panel texture ID: " << g_menu_textures.panel.id << "\n";
                return true;
            }
            catch (const std::exception& ex)
            {
                std::cerr << "Failed to load menu textures: " << ex.what() << '\n';
                return false;
            }
        }

        void destroy_menu_textures()
        {
            destroy_texture(g_menu_textures.panel);
            g_menu_textures.loaded = false;
        }

        void render_colored_quad(const RenderState& render_state, const glm::mat4& mvp, 
                                 float x, float y, float width, float height,
                                 float r, float g, float b, float a = 1.0f)
        {
            // Set uniforms - no texture
            glUniformMatrix4fv(render_state.mvp_location, 1, GL_FALSE, glm::value_ptr(mvp));
            glUniform1i(render_state.use_texture_location, 0);

            // Simple quad vertices (x, y, z, r, g, b, u, v)
            // No texture coordinates needed for colored quads
            const float vertices[] = {
                x,         y,          0.0f, r, g, b, a, 0.0f, 0.0f,  // Top-left
                x,         y + height, 0.0f, r, g, b, a, 0.0f, 0.0f,  // Bottom-left
                x + width, y + height, 0.0f, r, g, b, a, 0.0f, 0.0f,  // Bottom-right
                x,         y,          0.0f, r, g, b, a, 0.0f, 0.0f,  // Top-left
                x + width, y + height, 0.0f, r, g, b, a, 0.0f, 0.0f,  // Bottom-right
                x + width, y,          0.0f, r, g, b, a, 0.0f, 0.0f   // Top-right
            };

            static GLuint vao = 0, vbo = 0;
            if (vao == 0)
            {
                glGenVertexArrays(1, &vao);
                glGenBuffers(1, &vbo);
                glBindVertexArray(vao);
                glBindBuffer(GL_ARRAY_BUFFER, vbo);
                glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
                glEnableVertexAttribArray(0);
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
                // Use 4 components for color (r, g, b, a)
                glEnableVertexAttribArray(1);
                glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
                glEnableVertexAttribArray(2);
                glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(7 * sizeof(float)));
            }

            glBindVertexArray(vao);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glBindVertexArray(0);
        }

        void render_rounded_rect(const RenderState& render_state, const glm::mat4& mvp,
                                float x, float y, float width, float height, float radius,
                                float r, float g, float b, float a = 1.0f)
        {
            // For simplicity, render as regular rectangle with rounded corners approximated
            // We'll use a simple rectangle for now (can be improved with proper rounded rect shader)
            render_colored_quad(render_state, mvp, x, y, width, height, r, g, b, a);
        }

        void render_circle(const RenderState& render_state, const glm::mat4& mvp,
                          float center_x, float center_y, float radius,
                          float r, float g, float b, float a = 1.0f)
        {
            // Approximate circle with triangles
            const int segments = 32;
            const float angle_step = 2.0f * 3.14159265f / segments;
            
            std::vector<float> vertices;
            vertices.reserve(segments * 3 * 9); // 3 vertices per triangle, 9 floats per vertex
            
            for (int i = 0; i < segments; i++)
            {
                float angle1 = i * angle_step;
                float angle2 = (i + 1) * angle_step;
                
                // Center vertex
                vertices.push_back(center_x);
                vertices.push_back(center_y);
                vertices.push_back(0.0f);
                vertices.push_back(r);
                vertices.push_back(g);
                vertices.push_back(b);
                vertices.push_back(a);
                vertices.push_back(0.0f);
                vertices.push_back(0.0f);
                
                // First edge vertex
                vertices.push_back(center_x + radius * cosf(angle1));
                vertices.push_back(center_y + radius * sinf(angle1));
                vertices.push_back(0.0f);
                vertices.push_back(r);
                vertices.push_back(g);
                vertices.push_back(b);
                vertices.push_back(a);
                vertices.push_back(0.0f);
                vertices.push_back(0.0f);
                
                // Second edge vertex
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
            // Use 4 components for color (r, g, b, a)
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(7 * sizeof(float)));
            
            glUniformMatrix4fv(render_state.mvp_location, 1, GL_FALSE, glm::value_ptr(mvp));
            glUniform1i(render_state.use_texture_location, 0);
            
            glDrawArrays(GL_TRIANGLES, 0, segments * 3);
            glBindVertexArray(0);
        }

        void render_menu(const core::Window& window, const void* render_state_ptr,
                        const MenuState& /*menu_state*/)
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
            // Make it about 60% of window size
            const float popup_width = static_cast<float>(window_width) * 0.6f;
            const float popup_height = static_cast<float>(window_height) * 0.6f;
            const float popup_x = (static_cast<float>(window_width) - popup_width) * 0.5f;
            const float popup_y = (static_cast<float>(window_height) - popup_height) * 0.5f;
            
            // Panel colors from ColourMap.txt
            // PANEL BODY: #27234b (39, 35, 75)
            // PANEL TOP/BOTTOM: #1a1836 (26, 24, 54)
            const float panel_body_r = 39.0f / 255.0f;
            const float panel_body_g = 35.0f / 255.0f;
            const float panel_body_b = 75.0f / 255.0f;
            const float panel_alpha = 1.0f; // Fully opaque (not transparent)
            
            const float panel_top_r = 26.0f / 255.0f;
            const float panel_top_g = 24.0f / 255.0f;
            const float panel_top_b = 54.0f / 255.0f;
            
            // Render main panel body (rounded rectangle approximated as regular rectangle)
            render_rounded_rect(render_state, ui_mvp, popup_x, popup_y, popup_width, popup_height, 15.0f,
                                panel_body_r, panel_body_g, panel_body_b, panel_alpha);
            
            // Render header bar (top bar)
            const float header_height = 30.0f;
            render_colored_quad(render_state, ui_mvp, popup_x, popup_y, popup_width, header_height,
                               panel_top_r, panel_top_g, panel_top_b, panel_alpha);
            
            // Render bottom bar
            const float bottom_height = 10.0f;
            render_colored_quad(render_state, ui_mvp, popup_x, popup_y + popup_height - bottom_height, 
                               popup_width, bottom_height,
                               panel_top_r, panel_top_g, panel_top_b, panel_alpha);
            
            // Render window control buttons (red, orange, green) at top-left
            // From ColourMap.txt: RED DOT: #e15659, ORANGE DOT: #dfa328, GREEN DOT: #2bc642
            const float button_size = 12.0f;
            const float button_spacing = 4.0f;
            const float button_y = popup_y + 8.0f; // Top of panel
            float button_x = popup_x + 8.0f;
            
            // Red button (#e15659 = 225, 86, 89)
            render_circle(render_state, ui_mvp, button_x + button_size * 0.5f, button_y + button_size * 0.5f,
                         button_size * 0.5f, 225.0f/255.0f, 86.0f/255.0f, 89.0f/255.0f, panel_alpha);
            
            // Orange button (#dfa328 = 223, 163, 40)
            button_x += button_size + button_spacing;
            render_circle(render_state, ui_mvp, button_x + button_size * 0.5f, button_y + button_size * 0.5f,
                         button_size * 0.5f, 223.0f/255.0f, 163.0f/255.0f, 40.0f/255.0f, panel_alpha);
            
            // Green button (#2bc642 = 43, 198, 66)
            button_x += button_size + button_spacing;
            render_circle(render_state, ui_mvp, button_x + button_size * 0.5f, button_y + button_size * 0.5f,
                         button_size * 0.5f, 43.0f/255.0f, 198.0f/255.0f, 66.0f/255.0f, panel_alpha);

            // Render game title text in white
            // Set MVP matrix for text rendering (text renderer uses the same shader program)
            glUniformMatrix4fv(render_state.mvp_location, 1, GL_FALSE, glm::value_ptr(ui_mvp));
            glUniform1i(render_state.use_texture_location, 1); // Text uses texture
            if (render_state.dice_texture_mode_location >= 0)
            {
                glUniform1i(render_state.dice_texture_mode_location, 0); // Not dice texture mode
            }
            
            const float title_scale = 1.8f; // Smaller scale
            const float title_x = popup_x + popup_width * 0.5f; // Center horizontally
            const float title_y = popup_y + header_height + 60.0f; // Below header bar
            // Use pure white color (1.0, 1.0, 1.0)
            const glm::vec3 white_color(1.0f, 1.0f, 1.0f);
            
            // Render game title
            render_text(render_state.text_renderer, "SNAKES AND LADDERS", title_x, title_y, title_scale, white_color);
            
            // Render start game button
            // Button colors from ColourMap.txt: BUTTON BODY: #1f1c3b, BUTTON OUTLINE: #1a1836, BUTTON YELLOW: #eed512
            const float start_button_width = popup_width * 0.3f; // Shorter width
            const float start_button_height = 80.0f; // Taller height
            const float start_button_x = popup_x + (popup_width - start_button_width) * 0.5f; // Center horizontally
            const float start_button_y = popup_y + popup_height - start_button_height - 80.0f; // Near bottom
            
            // Button body color (#1f1c3b = 31, 28, 59)
            const float button_body_r = 31.0f / 255.0f;
            const float button_body_g = 28.0f / 255.0f;
            const float button_body_b = 59.0f / 255.0f;
            
            // Render button (rounded rectangle approximated as regular rectangle)
            render_rounded_rect(render_state, ui_mvp, start_button_x, start_button_y, start_button_width, start_button_height, 10.0f,
                                button_body_r, button_body_g, button_body_b, panel_alpha);
            
            // Render yellow underline at bottom of button (#eed512 = 238, 213, 18)
            const float underline_height = 4.0f;
            const float underline_y = start_button_y + start_button_height - underline_height;
            const float underline_r = 238.0f / 255.0f;
            const float underline_g = 213.0f / 255.0f;
            const float underline_b = 18.0f / 255.0f;
            render_colored_quad(render_state, ui_mvp, start_button_x, underline_y, start_button_width, underline_height,
                               underline_r, underline_g, underline_b, panel_alpha);
            
            // Render "PRESS ME" text on button
            glUniformMatrix4fv(render_state.mvp_location, 1, GL_FALSE, glm::value_ptr(ui_mvp));
            glUniform1i(render_state.use_texture_location, 1);
            if (render_state.dice_texture_mode_location >= 0)
            {
                glUniform1i(render_state.dice_texture_mode_location, 0);
            }
            
            const float button_text_scale = 1.0f; // Smaller scale to fit button
            const float button_text_x = start_button_x + start_button_width * 0.5f; // Center horizontally
            const float button_text_y = start_button_y + start_button_height * 0.5f - 10.0f; // Center vertically (slightly above center)
            render_text(render_state.text_renderer, "SPACE", button_text_x, button_text_y, button_text_scale, white_color);

            glDisable(GL_BLEND);
            glEnable(GL_DEPTH_TEST);
            glUniform1i(render_state.use_texture_location, 0);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }
}

