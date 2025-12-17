#pragma once

// Suppress IntelliSense error for GLM - the file exists, just IntelliSense can't find it
// This is a workaround until compile_commands.json is properly generated
#ifdef __INTELLISENSE__
// IntelliSense workaround - define minimal GLM types
namespace glm {
    template<typename T> struct vec2_impl {
        T x, y;
        vec2_impl() : x(0), y(0) {}
        vec2_impl(T x, T y) : x(x), y(y) {}
    };
    template<typename T> struct vec3_impl {
        T x, y, z;
        vec3_impl() : x(0), y(0), z(0) {}
        vec3_impl(T x, T y, T z) : x(x), y(y), z(z) {}
    };
    typedef vec2_impl<float> vec2;
    typedef vec3_impl<float> vec3;
}
#else
#include <glm/glm.hpp>
#endif

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

