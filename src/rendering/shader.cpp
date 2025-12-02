#include "shader.h"

#include <glad/glad.h>
#include <stdexcept>
#include <vector>

GLuint compile_shader(GLenum shader_type, const std::string& source)
{
    GLuint shader = glCreateShader(shader_type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE)
    {
        GLint log_length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
        std::vector<GLchar> info_log(static_cast<std::size_t>(log_length));
        glGetShaderInfoLog(shader, log_length, nullptr, info_log.data());

        std::string message(info_log.begin(), info_log.end());
        glDeleteShader(shader);
        throw std::runtime_error("Shader compilation failed: " + message);
    }

    return shader;
}

GLuint create_program(const std::string& vertex_source, const std::string& fragment_source)
{
    GLuint vertex_shader = compile_shader(GL_VERTEX_SHADER, vertex_source);
    GLuint fragment_shader = compile_shader(GL_FRAGMENT_SHADER, fragment_source);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    GLint success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (success == GL_FALSE)
    {
        GLint log_length = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);
        std::vector<GLchar> info_log(static_cast<std::size_t>(log_length));
        glGetProgramInfoLog(program, log_length, nullptr, info_log.data());

        glDeleteProgram(program);
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);

        std::string message(info_log.begin(), info_log.end());
        throw std::runtime_error("Program linking failed: " + message);
    }

    glDetachShader(program, vertex_shader);
    glDetachShader(program, fragment_shader);
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    return program;
}

