#pragma once

#include <glm/glm.hpp>
#include <string>
#include <map>

// Forward declarations for OpenGL types
typedef unsigned int GLuint;
typedef int GLsizei;

struct TextGlyph
{
    GLuint texture_id = 0;
    glm::ivec2 size = glm::ivec2(0);     // Size of glyph
    glm::ivec2 bearing = glm::ivec2(0);  // Offset from baseline to left/top of glyph
    GLuint advance = 0;                 // Offset to advance to next glyph
};

struct TextRenderer
{
    GLuint vao = 0;
    GLuint vbo = 0;
    std::map<char, TextGlyph> glyphs;
    bool initialized = false;
};

bool initialize_text_renderer(TextRenderer& renderer, const std::string& font_path, int pixel_height);
void destroy_text_renderer(TextRenderer& renderer);
void render_text(const TextRenderer& renderer, const std::string& text, float x, float y, float scale, const glm::vec3& color);




