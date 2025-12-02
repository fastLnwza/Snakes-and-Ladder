#include "text_renderer.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <glad/glad.h>

#include <iostream>

namespace
{
    constexpr int FIRST_CHAR = 32;
    constexpr int LAST_CHAR = 126;
}

bool initialize_text_renderer(TextRenderer& renderer, const std::string& font_path, int pixel_height)
{
    if (renderer.initialized)
    {
        destroy_text_renderer(renderer);
    }

    FT_Library ft;
    if (FT_Init_FreeType(&ft) != 0)
    {
        std::cerr << "Failed to initialize FreeType library\n";
        return false;
    }

    FT_Face face;
    if (FT_New_Face(ft, font_path.c_str(), 0, &face) != 0)
    {
        std::cerr << "Failed to load font: " << font_path << '\n';
        FT_Done_FreeType(ft);
        return false;
    }

    FT_Set_Pixel_Sizes(face, 0, pixel_height);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    int loaded_glyphs = 0;
    for (unsigned char c = FIRST_CHAR; c <= LAST_CHAR; ++c)
    {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER) != 0)
        {
            std::cerr << "Failed to load glyph for char " << static_cast<int>(c) << '\n';
            continue;
        }

        GLuint texture = 0;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_RED,
                     face->glyph->bitmap.width,
                     face->glyph->bitmap.rows,
                     0,
                     GL_RED,
                     GL_UNSIGNED_BYTE,
                     face->glyph->bitmap.buffer);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        TextGlyph glyph;
        glyph.texture_id = texture;
        glyph.size = glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows);
        glyph.bearing = glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top);
        glyph.advance = static_cast<GLuint>(face->glyph->advance.x);

        renderer.glyphs[static_cast<char>(c)] = glyph;
        loaded_glyphs++;
    }

    std::cout << "Loaded " << loaded_glyphs << " glyphs from font: " << font_path << std::endl;

    glBindTexture(GL_TEXTURE_2D, 0);
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    glGenVertexArrays(1, &renderer.vao);
    glGenBuffers(1, &renderer.vbo);
    glBindVertexArray(renderer.vao);
    glBindBuffer(GL_ARRAY_BUFFER, renderer.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 8, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void*>(6 * sizeof(float)));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    renderer.initialized = true;
    return true;
}

void destroy_text_renderer(TextRenderer& renderer)
{
    if (!renderer.initialized)
    {
        return;
    }

    for (auto& entry : renderer.glyphs)
    {
        glDeleteTextures(1, &entry.second.texture_id);
    }
    renderer.glyphs.clear();

    if (renderer.vbo != 0)
    {
        glDeleteBuffers(1, &renderer.vbo);
        renderer.vbo = 0;
    }
    if (renderer.vao != 0)
    {
        glDeleteVertexArrays(1, &renderer.vao);
        renderer.vao = 0;
    }

    renderer.initialized = false;
}

void render_text(const TextRenderer& renderer,
                 const std::string& text,
                 float x,
                 float y,
                 float scale,
                 const glm::vec3& color)
{
    if (!renderer.initialized)
    {
        return;
    }

    glBindVertexArray(renderer.vao);

    float total_width = 0.0f;
    for (char c : text)
    {
        auto glyph_it = renderer.glyphs.find(c);
        if (glyph_it == renderer.glyphs.end())
        {
            total_width += scale * 10.0f;
            continue;
        }
        total_width += static_cast<float>(glyph_it->second.advance >> 6) * scale;
    }

    float cursor_x = x - total_width * 0.5f;
    for (char c : text)
    {
        auto glyph_it = renderer.glyphs.find(c);
        if (glyph_it == renderer.glyphs.end())
        {
            cursor_x += scale * 10.0f;
            continue;
        }

        const TextGlyph& glyph = glyph_it->second;
        float xpos = cursor_x + static_cast<float>(glyph.bearing.x) * scale;
        float ypos = y - static_cast<float>(glyph.size.y - glyph.bearing.y) * scale;

        const float w = static_cast<float>(glyph.size.x) * scale;
        const float h = static_cast<float>(glyph.size.y) * scale;

        const float z = -0.5f;
        const float vertices[6][8] = {
            {xpos,     ypos + h, z, color.r, color.g, color.b, 0.0f, 1.0f},
            {xpos,     ypos,     z, color.r, color.g, color.b, 0.0f, 0.0f},
            {xpos + w, ypos,     z, color.r, color.g, color.b, 1.0f, 0.0f},

            {xpos,     ypos + h, z, color.r, color.g, color.b, 0.0f, 1.0f},
            {xpos + w, ypos,     z, color.r, color.g, color.b, 1.0f, 0.0f},
            {xpos + w, ypos + h, z, color.r, color.g, color.b, 1.0f, 1.0f}
        };

        glBindTexture(GL_TEXTURE_2D, glyph.texture_id);
        glBindBuffer(GL_ARRAY_BUFFER, renderer.vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        cursor_x += static_cast<float>(glyph.advance >> 6) * scale;
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}


