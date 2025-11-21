#pragma once

#include <glm/glm.hpp>
#include <limits>

// Forward declarations for OpenGL types
// glad.h should be included in main.cpp or the file that uses these types
typedef unsigned int GLuint;
typedef int GLsizei;

struct Vertex
{
    glm::vec3 position;
    glm::vec3 color;
    glm::vec2 texcoord = glm::vec2(0.0f, 0.0f); // Texture coordinates (optional, default to 0,0)
};

struct Mesh
{
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    GLsizei index_count = 0;
};

struct Bounds
{
    glm::vec3 min = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 max = glm::vec3(std::numeric_limits<float>::lowest());
};

