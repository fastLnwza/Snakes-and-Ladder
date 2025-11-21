#pragma once

#include <filesystem>
#include <string>

// Forward declarations for OpenGL types
typedef unsigned int GLuint;

struct Texture
{
    GLuint id = 0;
    int width = 0;
    int height = 0;
};

Texture load_texture(const std::filesystem::path& path);
void destroy_texture(Texture& texture);

