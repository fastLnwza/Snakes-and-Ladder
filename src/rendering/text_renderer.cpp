#include "text_renderer.h"

#include "mesh.h"
#include <glad/glad.h>
#include <algorithm>
#include <cmath>
#include <sstream>

void initialize_text_renderer(TextRenderer& renderer)
{
    renderer.initialized = true;
}

void destroy_text_renderer(TextRenderer& renderer)
{
    renderer.initialized = false;
}

// Create a simple digit mesh using quads
void create_digit_mesh(char digit, std::vector<Vertex>& vertices, std::vector<unsigned int>& indices,
                       float x, float y, float width, float height, const glm::vec3& color)
{
    // Create quads for digits - simplified approach
    auto add_quad = [&](float x1, float y1, float x2, float y2)
    {
        size_t base = vertices.size();
        // Use screen coordinates directly, z = -0.5 to ensure it's in front
        vertices.push_back({{x + x1 * width, y + y1 * height, -0.5f}, color, {0.0f, 0.0f}});
        vertices.push_back({{x + x2 * width, y + y1 * height, -0.5f}, color, {1.0f, 0.0f}});
        vertices.push_back({{x + x2 * width, y + y2 * height, -0.5f}, color, {1.0f, 1.0f}});
        vertices.push_back({{x + x1 * width, y + y2 * height, -0.5f}, color, {0.0f, 1.0f}});
        indices.push_back(base); indices.push_back(base + 1); indices.push_back(base + 2);
        indices.push_back(base + 2); indices.push_back(base + 3); indices.push_back(base);
    };
    
    const float line_width = 0.15f;
    
    switch (digit)
    {
        case '0':
            add_quad(0.1f, 0.0f, 0.9f, line_width);
            add_quad(0.1f, 1.0f - line_width, 0.9f, 1.0f);
            add_quad(0.0f, 0.0f, line_width, 1.0f);
            add_quad(1.0f - line_width, 0.0f, 1.0f, 1.0f);
            break;
        case '1':
            add_quad(0.4f, 0.0f, 0.6f, 1.0f);
            break;
        case '2':
            add_quad(0.1f, 0.0f, 0.9f, line_width);
            add_quad(0.9f - line_width, 0.0f, 0.9f, 0.5f);
            add_quad(0.1f, 0.5f - line_width * 0.5f, 0.9f, 0.5f + line_width * 0.5f);
            add_quad(0.0f, 0.5f, line_width, 1.0f);
            add_quad(0.1f, 1.0f - line_width, 0.9f, 1.0f);
            break;
        case '3':
            add_quad(0.1f, 0.0f, 0.9f, line_width);
            add_quad(0.9f - line_width, 0.0f, 0.9f, 1.0f);
            add_quad(0.1f, 0.5f - line_width * 0.5f, 0.9f, 0.5f + line_width * 0.5f);
            add_quad(0.1f, 1.0f - line_width, 0.9f, 1.0f);
            break;
        case '4':
            add_quad(0.0f, 0.0f, line_width, 0.5f);
            add_quad(0.1f, 0.5f - line_width * 0.5f, 0.9f, 0.5f + line_width * 0.5f);
            add_quad(0.9f - line_width, 0.0f, 0.9f, 1.0f);
            break;
        case '5':
            add_quad(0.1f, 0.0f, 0.9f, line_width);
            add_quad(0.0f, 0.0f, line_width, 0.5f);
            add_quad(0.1f, 0.5f - line_width * 0.5f, 0.9f, 0.5f + line_width * 0.5f);
            add_quad(0.9f - line_width, 0.5f, 0.9f, 1.0f);
            add_quad(0.1f, 1.0f - line_width, 0.9f, 1.0f);
            break;
        case '6':
            add_quad(0.1f, 0.0f, 0.9f, line_width);
            add_quad(0.0f, 0.0f, line_width, 1.0f);
            add_quad(0.1f, 0.5f - line_width * 0.5f, 0.9f, 0.5f + line_width * 0.5f);
            add_quad(0.9f - line_width, 0.5f, 0.9f, 1.0f);
            add_quad(0.1f, 1.0f - line_width, 0.9f, 1.0f);
            break;
        case '7':
            add_quad(0.1f, 0.0f, 0.9f, line_width);
            add_quad(0.9f - line_width, 0.0f, 0.9f, 1.0f);
            break;
        case '8':
            add_quad(0.1f, 0.0f, 0.9f, line_width);
            add_quad(0.0f, 0.0f, line_width, 1.0f);
            add_quad(0.9f - line_width, 0.0f, 0.9f, 1.0f);
            add_quad(0.1f, 0.5f - line_width * 0.5f, 0.9f, 0.5f + line_width * 0.5f);
            add_quad(0.1f, 1.0f - line_width, 0.9f, 1.0f);
            break;
        case '9':
            add_quad(0.1f, 0.0f, 0.9f, line_width);
            add_quad(0.0f, 0.0f, line_width, 0.5f);
            add_quad(0.9f - line_width, 0.0f, 0.9f, 1.0f);
            add_quad(0.1f, 0.5f - line_width * 0.5f, 0.9f, 0.5f + line_width * 0.5f);
            add_quad(0.1f, 1.0f - line_width, 0.9f, 1.0f);
            break;
        default:
            break;
    }
}

void render_text(const TextRenderer& renderer, const std::string& text, float x, float y, float scale,
                 const glm::vec3& color, int window_width, int window_height)
{
    if (!renderer.initialized)
        return;
    
    // Use screen coordinates directly (no NDC conversion needed)
    // x and y are already in screen pixel coordinates
    
    // Create mesh for entire text
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    
    float char_width = 20.0f * scale; // Use pixel units
    float char_height = 30.0f * scale;
    float char_spacing = char_width * 1.2f;
    float current_x = x;
    
    // Center text horizontally
    float text_width = text.length() * char_spacing;
    current_x -= text_width * 0.5f;
    
    // Helper function to add a quad for letter rendering
    auto add_letter_quad = [&](float x1, float y1, float x2, float y2, float base_x, float base_y, float w, float h)
    {
        size_t base_idx = vertices.size();
        vertices.push_back({{base_x + x1 * w, base_y + y1 * h, -0.5f}, color, {0.0f, 0.0f}});
        vertices.push_back({{base_x + x2 * w, base_y + y1 * h, -0.5f}, color, {1.0f, 0.0f}});
        vertices.push_back({{base_x + x2 * w, base_y + y2 * h, -0.5f}, color, {1.0f, 1.0f}});
        vertices.push_back({{base_x + x1 * w, base_y + y2 * h, -0.5f}, color, {0.0f, 1.0f}});
        indices.push_back(base_idx); indices.push_back(base_idx + 1); indices.push_back(base_idx + 2);
        indices.push_back(base_idx + 2); indices.push_back(base_idx + 3); indices.push_back(base_idx);
    };
    
    for (char c : text)
    {
        float char_base_x = current_x;
        float char_base_y = y - char_height * 0.5f;
        const float line_w = 0.12f;
        
        if (c >= '0' && c <= '9')
        {
            create_digit_mesh(c, vertices, indices, char_base_x, char_base_y,
                            char_width, char_height, color);
        }
        else if (c >= 'A' && c <= 'Z')
        {
            // Render uppercase letters using simple block style
            char upper_c = c;
            if (upper_c == 'M')
            {
                // M
                add_letter_quad(0.0f, 0.0f, line_w, 1.0f, char_base_x, char_base_y, char_width, char_height);
                add_letter_quad(0.5f - line_w * 0.5f, 0.3f, 0.5f + line_w * 0.5f, 0.7f, char_base_x, char_base_y, char_width, char_height);
                add_letter_quad(1.0f - line_w, 0.0f, 1.0f, 1.0f, char_base_x, char_base_y, char_width, char_height);
                add_letter_quad(0.0f, 0.0f, 1.0f, line_w, char_base_x, char_base_y, char_width, char_height);
            }
            else if (upper_c == 'I')
            {
                // I
                add_letter_quad(0.4f, 0.0f, 0.6f, 1.0f, char_base_x, char_base_y, char_width, char_height);
                add_letter_quad(0.0f, 0.0f, 1.0f, line_w, char_base_x, char_base_y, char_width, char_height);
                add_letter_quad(0.0f, 1.0f - line_w, 1.0f, 1.0f, char_base_x, char_base_y, char_width, char_height);
            }
            else if (upper_c == 'S')
            {
                // S
                add_letter_quad(0.1f, 0.0f, 0.9f, line_w, char_base_x, char_base_y, char_width, char_height);
                add_letter_quad(0.0f, 0.0f, line_w, 0.5f, char_base_x, char_base_y, char_width, char_height);
                add_letter_quad(0.1f, 0.5f - line_w * 0.5f, 0.9f, 0.5f + line_w * 0.5f, char_base_x, char_base_y, char_width, char_height);
                add_letter_quad(0.9f - line_w, 0.5f, 0.9f, 1.0f, char_base_x, char_base_y, char_width, char_height);
                add_letter_quad(0.1f, 1.0f - line_w, 0.9f, 1.0f, char_base_x, char_base_y, char_width, char_height);
            }
            else if (upper_c == 'F')
            {
                // F
                add_letter_quad(0.0f, 0.0f, line_w, 1.0f, char_base_x, char_base_y, char_width, char_height);
                add_letter_quad(0.1f, 0.0f, 0.9f, line_w, char_base_x, char_base_y, char_width, char_height);
                add_letter_quad(0.1f, 0.5f - line_w * 0.5f, 0.7f, 0.5f + line_w * 0.5f, char_base_x, char_base_y, char_width, char_height);
            }
            else if (upper_c == 'A')
            {
                // A
                add_letter_quad(0.0f, 0.0f, line_w, 1.0f, char_base_x, char_base_y, char_width, char_height);
                add_letter_quad(0.9f - line_w, 0.0f, 0.9f, 1.0f, char_base_x, char_base_y, char_width, char_height);
                add_letter_quad(0.1f, 0.0f, 0.9f, line_w, char_base_x, char_base_y, char_width, char_height);
                add_letter_quad(0.1f, 0.5f - line_w * 0.5f, 0.9f, 0.5f + line_w * 0.5f, char_base_x, char_base_y, char_width, char_height);
            }
            else if (upper_c == 'L')
            {
                // L
                add_letter_quad(0.0f, 0.0f, line_w, 1.0f, char_base_x, char_base_y, char_width, char_height);
                add_letter_quad(0.1f, 1.0f - line_w, 0.9f, 1.0f, char_base_x, char_base_y, char_width, char_height);
            }
            else if (upper_c == 'N')
            {
                // N
                add_letter_quad(0.0f, 0.0f, line_w, 1.0f, char_base_x, char_base_y, char_width, char_height);
                add_letter_quad(0.5f - line_w * 0.5f, 0.2f, 0.5f + line_w * 0.5f, 0.8f, char_base_x, char_base_y, char_width, char_height);
                add_letter_quad(0.9f - line_w, 0.0f, 0.9f, 1.0f, char_base_x, char_base_y, char_width, char_height);
            }
            else if (upper_c == 'O')
            {
                // O
                add_letter_quad(0.1f, 0.0f, 0.9f, line_w, char_base_x, char_base_y, char_width, char_height);
                add_letter_quad(0.0f, 0.0f, line_w, 1.0f, char_base_x, char_base_y, char_width, char_height);
                add_letter_quad(0.9f - line_w, 0.0f, 0.9f, 1.0f, char_base_x, char_base_y, char_width, char_height);
                add_letter_quad(0.1f, 1.0f - line_w, 0.9f, 1.0f, char_base_x, char_base_y, char_width, char_height);
            }
            else if (upper_c == 'E')
            {
                // E
                add_letter_quad(0.0f, 0.0f, line_w, 1.0f, char_base_x, char_base_y, char_width, char_height);
                add_letter_quad(0.1f, 0.0f, 0.9f, line_w, char_base_x, char_base_y, char_width, char_height);
                add_letter_quad(0.1f, 0.5f - line_w * 0.5f, 0.7f, 0.5f + line_w * 0.5f, char_base_x, char_base_y, char_width, char_height);
                add_letter_quad(0.1f, 1.0f - line_w, 0.9f, 1.0f, char_base_x, char_base_y, char_width, char_height);
            }
            else if (upper_c == 'B')
            {
                // B (Bonus)
                add_letter_quad(0.0f, 0.0f, line_w, 1.0f, char_base_x, char_base_y, char_width, char_height);
                add_letter_quad(0.1f, 0.0f, 0.8f, line_w, char_base_x, char_base_y, char_width, char_height);
                add_letter_quad(0.8f - line_w, 0.0f, 0.8f, 0.5f, char_base_x, char_base_y, char_width, char_height);
                add_letter_quad(0.1f, 0.5f - line_w * 0.5f, 0.8f, 0.5f + line_w * 0.5f, char_base_x, char_base_y, char_width, char_height);
                add_letter_quad(0.8f - line_w, 0.5f, 0.8f, 1.0f, char_base_x, char_base_y, char_width, char_height);
                add_letter_quad(0.1f, 1.0f - line_w, 0.8f, 1.0f, char_base_x, char_base_y, char_width, char_height);
            }
            else if (upper_c == 'U')
            {
                // U
                add_letter_quad(0.0f, 0.0f, line_w, 1.0f - line_w, char_base_x, char_base_y, char_width, char_height);
                add_letter_quad(0.9f - line_w, 0.0f, 0.9f, 1.0f - line_w, char_base_x, char_base_y, char_width, char_height);
                add_letter_quad(0.1f, 1.0f - line_w, 0.9f, 1.0f, char_base_x, char_base_y, char_width, char_height);
            }
            // For other uppercase letters, render as simple blocks (fallback)
            else
            {
                add_letter_quad(0.1f, 0.0f, 0.9f, 1.0f, char_base_x, char_base_y, char_width, char_height);
            }
        }
        else if (c >= 'a' && c <= 'z')
        {
            // Convert lowercase to uppercase for rendering
            char upper_c = c - 'a' + 'A';
            // Render lowercase letters (same shapes as uppercase, just smaller or different position)
            if (upper_c == 'I' || c == 'i')
            {
                add_letter_quad(0.4f, 0.0f, 0.6f, 1.0f, char_base_x, char_base_y, char_width, char_height);
                add_letter_quad(0.0f, 0.0f, 1.0f, line_w, char_base_x, char_base_y, char_width, char_height);
                add_letter_quad(0.0f, 1.0f - line_w, 1.0f, 1.0f, char_base_x, char_base_y, char_width, char_height);
            }
            else if (upper_c == 'S' || c == 's')
            {
                add_letter_quad(0.1f, 0.0f, 0.9f, line_w, char_base_x, char_base_y, char_width, char_height);
                add_letter_quad(0.0f, 0.0f, line_w, 0.5f, char_base_x, char_base_y, char_width, char_height);
                add_letter_quad(0.1f, 0.5f - line_w * 0.5f, 0.9f, 0.5f + line_w * 0.5f, char_base_x, char_base_y, char_width, char_height);
                add_letter_quad(0.9f - line_w, 0.5f, 0.9f, 1.0f, char_base_x, char_base_y, char_width, char_height);
                add_letter_quad(0.1f, 1.0f - line_w, 0.9f, 1.0f, char_base_x, char_base_y, char_width, char_height);
            }
            else if (upper_c == 'O' || c == 'o')
            {
                add_letter_quad(0.1f, 0.0f, 0.9f, line_w, char_base_x, char_base_y, char_width, char_height);
                add_letter_quad(0.0f, 0.0f, line_w, 1.0f, char_base_x, char_base_y, char_width, char_height);
                add_letter_quad(0.9f - line_w, 0.0f, 0.9f, 1.0f, char_base_x, char_base_y, char_width, char_height);
                add_letter_quad(0.1f, 1.0f - line_w, 0.9f, 1.0f, char_base_x, char_base_y, char_width, char_height);
            }
            else if (upper_c == 'N' || c == 'n')
            {
                add_letter_quad(0.0f, 0.0f, line_w, 1.0f, char_base_x, char_base_y, char_width, char_height);
                add_letter_quad(0.5f - line_w * 0.5f, 0.2f, 0.5f + line_w * 0.5f, 0.8f, char_base_x, char_base_y, char_width, char_height);
                add_letter_quad(0.9f - line_w, 0.0f, 0.9f, 1.0f, char_base_x, char_base_y, char_width, char_height);
            }
            else if (upper_c == 'L' || c == 'l')
            {
                add_letter_quad(0.0f, 0.0f, line_w, 1.0f, char_base_x, char_base_y, char_width, char_height);
                add_letter_quad(0.1f, 1.0f - line_w, 0.9f, 1.0f, char_base_x, char_base_y, char_width, char_height);
            }
            else if (upper_c == 'A' || c == 'a')
            {
                add_letter_quad(0.0f, 0.0f, line_w, 1.0f, char_base_x, char_base_y, char_width, char_height);
                add_letter_quad(0.9f - line_w, 0.0f, 0.9f, 1.0f, char_base_x, char_base_y, char_width, char_height);
                add_letter_quad(0.1f, 0.0f, 0.9f, line_w, char_base_x, char_base_y, char_width, char_height);
                add_letter_quad(0.1f, 0.5f - line_w * 0.5f, 0.9f, 0.5f + line_w * 0.5f, char_base_x, char_base_y, char_width, char_height);
            }
            else if (upper_c == 'F' || c == 'f')
            {
                add_letter_quad(0.0f, 0.0f, line_w, 1.0f, char_base_x, char_base_y, char_width, char_height);
                add_letter_quad(0.1f, 0.0f, 0.9f, line_w, char_base_x, char_base_y, char_width, char_height);
                add_letter_quad(0.1f, 0.5f - line_w * 0.5f, 0.7f, 0.5f + line_w * 0.5f, char_base_x, char_base_y, char_width, char_height);
            }
            else
            {
                // Fallback for other lowercase - render as simple block
                add_letter_quad(0.2f, 0.2f, 0.8f, 0.8f, char_base_x, char_base_y, char_width, char_height);
            }
        }
        else if (c == ':')
        {
            // Draw colon as two dots
            size_t base = vertices.size();
            float dot_size = char_width * 0.3f;
            float dot1_y = y - char_height * 0.5f + char_height * 0.3f;
            float dot2_y = y - char_height * 0.5f + char_height * 0.7f;
            float dot_x = current_x + char_width * 0.5f - dot_size * 0.5f;
            
            vertices.push_back({{dot_x, dot1_y, -0.5f}, color, {0.0f, 0.0f}});
            vertices.push_back({{dot_x + dot_size, dot1_y, -0.5f}, color, {1.0f, 0.0f}});
            vertices.push_back({{dot_x + dot_size, dot1_y + dot_size, -0.5f}, color, {1.0f, 1.0f}});
            vertices.push_back({{dot_x, dot1_y + dot_size, -0.5f}, color, {0.0f, 1.0f}});
            indices.push_back(base); indices.push_back(base + 1); indices.push_back(base + 2);
            indices.push_back(base + 2); indices.push_back(base + 3); indices.push_back(base);
            
            base = vertices.size();
            vertices.push_back({{dot_x, dot2_y, -0.5f}, color, {0.0f, 0.0f}});
            vertices.push_back({{dot_x + dot_size, dot2_y, -0.5f}, color, {1.0f, 0.0f}});
            vertices.push_back({{dot_x + dot_size, dot2_y + dot_size, -0.5f}, color, {1.0f, 1.0f}});
            vertices.push_back({{dot_x, dot2_y + dot_size, -0.5f}, color, {0.0f, 1.0f}});
            indices.push_back(base); indices.push_back(base + 1); indices.push_back(base + 2);
            indices.push_back(base + 2); indices.push_back(base + 3); indices.push_back(base);
        }
        else if (c == '+')
        {
            // Draw plus sign
            add_letter_quad(0.4f, 0.0f, 0.6f, 1.0f, char_base_x, char_base_y, char_width, char_height);
            add_letter_quad(0.1f, 0.45f, 0.9f, 0.55f, char_base_x, char_base_y, char_width, char_height);
        }
        else if (c == ' ')
        {
            // Skip spaces, just advance
        }
        current_x += char_spacing;
    }
    
    if (vertices.empty())
        return;
    
    // Create and render mesh
    Mesh text_mesh = create_mesh(vertices, indices);
    
    // Use identity matrices for orthographic rendering
    glBindVertexArray(text_mesh.vao);
    glDrawElements(GL_TRIANGLES, text_mesh.index_count, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
    
    destroy_mesh(text_mesh);
}
