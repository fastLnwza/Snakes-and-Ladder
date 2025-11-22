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
    
    for (char c : text)
    {
        if (c >= '0' && c <= '9')
        {
            create_digit_mesh(c, vertices, indices, current_x, y - char_height * 0.5f,
                            char_width, char_height, color);
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
