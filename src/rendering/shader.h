#pragma once

#include <string>

// Forward declarations for OpenGL types
// glad.h should be included in the implementation file
typedef unsigned int GLuint;
typedef unsigned int GLenum;

GLuint compile_shader(GLenum shader_type, const std::string& source);
GLuint create_program(const std::string& vertex_source, const std::string& fragment_source);

