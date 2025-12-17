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

    // Set texture parameters for proper alpha handling
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Determine format based on channels
    GLenum format = GL_RGB;
    GLenum internal_format = GL_RGB;
    unsigned char* rgba_data = nullptr;
    
    if (channels == 1)
    {
        format = GL_RED;
        internal_format = GL_RED;
    }
    else if (channels == 3)
    {
        // Convert RGB to RGBA by adding alpha channel
        // For UI textures, we want to make white/near-white pixels transparent
        rgba_data = new unsigned char[width * height * 4];
        for (int i = 0; i < width * height; i++)
        {
            unsigned char r = data[i * 3 + 0];
            unsigned char g = data[i * 3 + 1];
            unsigned char b = data[i * 3 + 2];
            
            // If pixel is white or near-white, make it transparent
            // Use a wider threshold (230) to catch more white/light pixels
            // Check if all channels are similar and high (white/light gray)
            bool is_white = (r > 230 && g > 230 && b > 230);
            // Also check if it's a very light color (high average)
            float avg = (r + g + b) / 3.0f;
            bool is_very_light = (avg > 230.0f);
            // Check if all channels are very close to each other (grayscale white)
            int diff_rg = abs((int)r - (int)g);
            int diff_gb = abs((int)g - (int)b);
            bool is_grayscale_white = (diff_rg < 10 && diff_gb < 10 && avg > 230.0f);
            
            unsigned char alpha = (is_white || is_very_light || is_grayscale_white) ? 0 : 255;
            
            rgba_data[i * 4 + 0] = r;
            rgba_data[i * 4 + 1] = g;
            rgba_data[i * 4 + 2] = b;
            rgba_data[i * 4 + 3] = alpha;
        }
        format = GL_RGBA;
        internal_format = GL_RGBA;
    }
    else if (channels == 4)
    {
        format = GL_RGBA;
        internal_format = GL_RGBA;
    }

    unsigned char* upload_data = (rgba_data != nullptr) ? rgba_data : data;
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, format, GL_UNSIGNED_BYTE, upload_data);
    glGenerateMipmap(GL_TEXTURE_2D);
    
    if (rgba_data != nullptr)
    {
        delete[] rgba_data;
    }

    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);

    std::cout << "Loaded texture: " << path.string() << " (" << width << "x" << height << ", " << channels << " channels";
    if (rgba_data != nullptr)
    {
        std::cout << " - converted to RGBA with white-to-transparent)";
    }
    std::cout << "\n";
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



