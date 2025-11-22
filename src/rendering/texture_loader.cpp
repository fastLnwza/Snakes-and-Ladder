#include "texture_loader.h"

#include <glad/glad.h>
#include <stdexcept>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "../../external/stb_image.h"

Texture load_texture(const std::filesystem::path& path)
{
    Texture texture{};

    int width, height, channels;
    unsigned char* data = stbi_load(path.string().c_str(), &width, &height, &channels, 0);
    
    if (!data)
    {
        throw std::runtime_error("Failed to load texture: " + path.string() + " - " + stbi_failure_reason());
    }

    texture.width = width;
    texture.height = height;

    glGenTextures(1, &texture.id);
    glBindTexture(GL_TEXTURE_2D, texture.id);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Determine format based on channels
    GLenum format = GL_RGB;
    if (channels == 1)
        format = GL_RED;
    else if (channels == 3)
        format = GL_RGB;
    else if (channels == 4)
        format = GL_RGBA;

    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);

    std::cout << "Loaded texture: " << path.string() << " (" << width << "x" << height << ", " << channels << " channels)\n";
    return texture;
}

void destroy_texture(Texture& texture)
{
    if (texture.id != 0)
    {
        glDeleteTextures(1, &texture.id);
        texture.id = 0;
    }
    texture.width = 0;
    texture.height = 0;
}



