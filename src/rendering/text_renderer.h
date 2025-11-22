#pragma once

#include "core/types.h"
#include <glm/glm.hpp>
#include <string>

struct TextRenderer
{
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    bool initialized = false;
};

void initialize_text_renderer(TextRenderer& renderer);
void destroy_text_renderer(TextRenderer& renderer);
void render_text(const TextRenderer& renderer, const std::string& text, float x, float y, float scale, const glm::vec3& color, int window_width, int window_height);

