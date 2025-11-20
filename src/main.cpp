#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/epsilon.hpp>

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

namespace
{
float g_camera_fov = 50.0f;

std::string load_file(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file)
    {
        throw std::runtime_error("Failed to open file: " + path.string());
    }

    std::string contents;
    file.seekg(0, std::ios::end);
    contents.resize(static_cast<std::size_t>(file.tellg()));
    file.seekg(0, std::ios::beg);
    file.read(contents.data(), static_cast<std::streamsize>(contents.size()));
    return contents;
}

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

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    (void)window;
    glViewport(0, 0, width, height);
}

void scroll_callback(GLFWwindow* window, double /*x_offset*/, double y_offset)
{
    (void)window;
    g_camera_fov = glm::clamp(g_camera_fov - static_cast<float>(y_offset) * 5.0f, 30.0f, 85.0f);
}
} // namespace

struct Vertex
{
    glm::vec3 position;
    glm::vec3 color;
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

void expand_bounds(Bounds& bounds, const glm::vec3& point)
{
    bounds.min = glm::min(bounds.min, point);
    bounds.max = glm::max(bounds.max, point);
}

Mesh create_mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices)
{
    Mesh mesh{};

    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glGenBuffers(1, &mesh.ebo);

    glBindVertexArray(mesh.vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)),
                 vertices.data(),
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
                 indices.data(),
                 GL_STATIC_DRAW);

    constexpr GLuint position_location = 0;
    constexpr GLuint color_location = 1;
    constexpr GLsizei stride = static_cast<GLsizei>(sizeof(Vertex));
    glVertexAttribPointer(position_location, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(position_location);
    glVertexAttribPointer(color_location,
                          3,
                          GL_FLOAT,
                          GL_FALSE,
                          stride,
                          reinterpret_cast<void*>(offsetof(Vertex, color)));
    glEnableVertexAttribArray(color_location);

    glBindVertexArray(0);

    mesh.index_count = static_cast<GLsizei>(indices.size());
    return mesh;
}

void destroy_mesh(Mesh& mesh)
{
    if (mesh.ebo != 0)
    {
        glDeleteBuffers(1, &mesh.ebo);
        mesh.ebo = 0;
    }
    if (mesh.vbo != 0)
    {
        glDeleteBuffers(1, &mesh.vbo);
        mesh.vbo = 0;
    }
    if (mesh.vao != 0)
    {
        glDeleteVertexArrays(1, &mesh.vao);
        mesh.vao = 0;
    }
    mesh.index_count = 0;
}

std::pair<std::vector<Vertex>, std::vector<unsigned int>> build_plane(float length,
                                                                      float width,
                                                                      const glm::vec3& color_center,
                                                                      const glm::vec3& color_edge)
{
    const float half_length = length * 0.5f;
    const float half_width = width * 0.5f;

    std::vector<Vertex> vertices = {
        {{-half_width, 0.0f, -half_length}, color_edge},
        {{half_width, 0.0f, -half_length}, color_edge},
        {{half_width, 0.0f, half_length}, color_edge},
        {{-half_width, 0.0f, half_length}, color_edge}};

    // lighten the center lane slightly
    vertices[1].color = color_center;
    vertices[2].color = color_center;

    std::vector<unsigned int> indices = {0, 1, 2, 2, 3, 0};

    return {vertices, indices};
}

bool load_gltf_model(const std::filesystem::path& gltf_path,
                     Mesh& out_mesh,
                     Bounds& out_bounds,
                     glm::vec3 default_color,
                     float target_extent)
{
    if (!std::filesystem::exists(gltf_path))
    {
        std::cerr << "GLTF file not found: " << gltf_path << '\n';
        return false;
    }

    cgltf_options options{};
    cgltf_data* data = nullptr;
    const std::string path_string = gltf_path.u8string();
    cgltf_result result = cgltf_parse_file(&options, path_string.c_str(), &data);
    if (result != cgltf_result_success)
    {
        std::cerr << "Failed to parse glTF file: " << gltf_path << " (error code " << static_cast<int>(result)
                  << ")\n";
        return false;
    }

    result = cgltf_load_buffers(&options, data, path_string.c_str());
    if (result != cgltf_result_success)
    {
        std::cerr << "Failed to load glTF buffers: " << gltf_path << " (error code " << static_cast<int>(result)
                  << ")\n";
        cgltf_free(data);
        return false;
    }

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    Bounds raw_bounds{};

    for (cgltf_size node_index = 0; node_index < data->nodes_count; ++node_index)
    {
        const cgltf_node& node = data->nodes[node_index];
        if (node.mesh == nullptr)
        {
            continue;
        }

        float transform_data[16];
        cgltf_node_transform_world(&node, transform_data);
        const glm::mat4 node_transform = glm::make_mat4(transform_data);

        const cgltf_mesh& mesh = *node.mesh;
        for (cgltf_size prim_index = 0; prim_index < mesh.primitives_count; ++prim_index)
        {
            const cgltf_primitive& primitive = mesh.primitives[prim_index];
            if (primitive.type != cgltf_primitive_type_triangles)
            {
                continue;
            }

            const cgltf_accessor* position_accessor = nullptr;
            const cgltf_accessor* color_accessor = nullptr;

            glm::vec3 primitive_color = default_color;
            if (primitive.material != nullptr)
            {
                const cgltf_material& material = *primitive.material;
                if (material.has_pbr_metallic_roughness)
                {
                    primitive_color = glm::vec3(material.pbr_metallic_roughness.base_color_factor[0],
                                                material.pbr_metallic_roughness.base_color_factor[1],
                                                material.pbr_metallic_roughness.base_color_factor[2]);
                }
                else if (material.has_pbr_specular_glossiness)
                {
                    primitive_color = glm::vec3(material.pbr_specular_glossiness.diffuse_factor[0],
                                                material.pbr_specular_glossiness.diffuse_factor[1],
                                                material.pbr_specular_glossiness.diffuse_factor[2]);
                }
                else if (material.emissive_factor[0] != 0.0f || material.emissive_factor[1] != 0.0f ||
                         material.emissive_factor[2] != 0.0f)
                {
                    primitive_color = glm::vec3(material.emissive_factor[0],
                                                material.emissive_factor[1],
                                                material.emissive_factor[2]);
                }
            }

            for (cgltf_size attr_index = 0; attr_index < primitive.attributes_count; ++attr_index)
            {
                const cgltf_attribute& attribute = primitive.attributes[attr_index];
                if (attribute.type == cgltf_attribute_type_position)
                {
                    position_accessor = attribute.data;
                }
                else if (attribute.type == cgltf_attribute_type_color)
                {
                    color_accessor = attribute.data;
                }
            }

            if (position_accessor == nullptr)
            {
                continue;
            }

            const cgltf_size vertex_count = position_accessor->count;
            const std::size_t vertex_offset = vertices.size();
            vertices.resize(vertex_offset + static_cast<std::size_t>(vertex_count));

            for (cgltf_size vertex_index = 0; vertex_index < vertex_count; ++vertex_index)
            {
                float position_data[3] = {0.0f, 0.0f, 0.0f};
                cgltf_accessor_read_float(position_accessor, vertex_index, position_data, 3);
                const glm::vec4 transformed =
                    node_transform * glm::vec4(position_data[0], position_data[1], position_data[2], 1.0f);
                const glm::vec3 position = glm::vec3(transformed);

                glm::vec3 color = primitive_color;
                if (color_accessor != nullptr)
                {
                    float color_data[4] = {default_color.r, default_color.g, default_color.b, 1.0f};
                    cgltf_accessor_read_float(color_accessor, vertex_index, color_data, 4);
                    color = glm::vec3(color_data[0], color_data[1], color_data[2]);
                }

                vertices[vertex_offset + static_cast<std::size_t>(vertex_index)] = {position, color};
                expand_bounds(raw_bounds, position);
            }

            if (primitive.indices != nullptr)
            {
                const cgltf_accessor* index_accessor = primitive.indices;
                const cgltf_size index_count = index_accessor->count;
                indices.reserve(indices.size() + static_cast<std::size_t>(index_count));

                for (cgltf_size index = 0; index < index_count; ++index)
                {
                    const cgltf_size value = cgltf_accessor_read_index(index_accessor, index);
                    indices.push_back(static_cast<unsigned int>(vertex_offset + value));
                }
            }
            else
            {
                for (cgltf_size index = 0; index < vertex_count; index += 3)
                {
                    indices.push_back(static_cast<unsigned int>(vertex_offset + index + 0));
                    indices.push_back(static_cast<unsigned int>(vertex_offset + index + 1));
                    indices.push_back(static_cast<unsigned int>(vertex_offset + index + 2));
                }
            }
        }
    }

    cgltf_free(data);

    if (vertices.empty() || indices.empty())
    {
        std::cerr << "glTF contained no renderable geometry: " << gltf_path << '\n';
        return false;
    }

    float scale = 1.0f;
    if (target_extent > 0.0f)
    {
        const float width = raw_bounds.max.x - raw_bounds.min.x;
        const float length = raw_bounds.max.z - raw_bounds.min.z;
        const float max_dimension = std::max(width, length);
        if (max_dimension > 0.0f)
        {
            scale = target_extent / max_dimension;
        }
    }

    if (!glm::epsilonEqual(scale, 1.0f, 1e-4f))
    {
        for (auto& vertex : vertices)
        {
            vertex.position *= scale;
        }
        raw_bounds.min *= scale;
        raw_bounds.max *= scale;
    }

    out_bounds = raw_bounds;

    out_mesh = create_mesh(vertices, indices);
    return true;
}

std::pair<std::vector<Vertex>, std::vector<unsigned int>> build_sphere(float radius,
                                                                       int sector_count,
                                                                       int stack_count,
                                                                       const glm::vec3& color)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    vertices.reserve(static_cast<std::size_t>((stack_count + 1) * (sector_count + 1)));
    indices.reserve(static_cast<std::size_t>(stack_count * sector_count * 6));

    const float sector_step = glm::two_pi<float>() / static_cast<float>(sector_count);
    const float stack_step = glm::pi<float>() / static_cast<float>(stack_count);

    for (int i = 0; i <= stack_count; ++i)
    {
        const float stack_angle = glm::half_pi<float>() - static_cast<float>(i) * stack_step;
        const float xy = radius * std::cos(stack_angle);
        const float y = radius * std::sin(stack_angle);

        for (int j = 0; j <= sector_count; ++j)
        {
            const float sector_angle = static_cast<float>(j) * sector_step;
            const float x = xy * std::cos(sector_angle);
            const float z = xy * std::sin(sector_angle);

            vertices.push_back({{x, y, z}, color});
        }
    }

    for (int i = 0; i < stack_count; ++i)
    {
        int k1 = i * (sector_count + 1);
        int k2 = k1 + sector_count + 1;

        for (int j = 0; j < sector_count; ++j, ++k1, ++k2)
        {
            if (i != 0)
            {
                indices.push_back(static_cast<unsigned int>(k1));
                indices.push_back(static_cast<unsigned int>(k2));
                indices.push_back(static_cast<unsigned int>(k1 + 1));
            }

            if (i != (stack_count - 1))
            {
                indices.push_back(static_cast<unsigned int>(k1 + 1));
                indices.push_back(static_cast<unsigned int>(k2));
                indices.push_back(static_cast<unsigned int>(k2 + 1));
            }
        }
    }

    return {vertices, indices};
}

void append_box_prism(std::vector<Vertex>& vertices,
                      std::vector<unsigned int>& indices,
                      float center_x,
                      float center_z,
                      float width,
                      float length,
                      float height,
             const glm::vec3& color)
{
    const float half_width = width * 0.5f;
    const float half_length = length * 0.5f;

    const std::array<glm::vec3, 8> corners = {{
        {center_x - half_width, 0.0f, center_z - half_length},
        {center_x + half_width, 0.0f, center_z - half_length},
        {center_x + half_width, 0.0f, center_z + half_length},
        {center_x - half_width, 0.0f, center_z + half_length},
        {center_x - half_width, height, center_z - half_length},
        {center_x + half_width, height, center_z - half_length},
        {center_x + half_width, height, center_z + half_length},
        {center_x - half_width, height, center_z + half_length},
    }};

    const std::array<std::array<int, 4>, 6> faces = {{
        {{0, 1, 2, 3}},
        {{4, 5, 6, 7}},
        {{0, 1, 5, 4}},
        {{1, 2, 6, 5}},
        {{2, 3, 7, 6}},
        {{3, 0, 4, 7}},
    }};

    for (const auto& face : faces)
    {
        const std::size_t offset = vertices.size();
        vertices.push_back({corners[face[0]], color});
        vertices.push_back({corners[face[1]], color});
        vertices.push_back({corners[face[2]], color});
        vertices.push_back({corners[face[3]], color});

        indices.push_back(static_cast<unsigned int>(offset + 0));
        indices.push_back(static_cast<unsigned int>(offset + 1));
        indices.push_back(static_cast<unsigned int>(offset + 2));
        indices.push_back(static_cast<unsigned int>(offset + 2));
        indices.push_back(static_cast<unsigned int>(offset + 3));
        indices.push_back(static_cast<unsigned int>(offset + 0));
    }
}

std::pair<std::vector<Vertex>, std::vector<unsigned int>> build_cube(float size, const glm::vec3& color)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    append_box_prism(vertices, indices, 0.0f, 0.0f, size, size, size, color);
    return {vertices, indices};
}

void append_oriented_prism(std::vector<Vertex>& vertices,
                           std::vector<unsigned int>& indices,
                           const glm::vec3& center,
                           const glm::vec3& right_dir,
                           const glm::vec3& up_dir,
                           const glm::vec3& forward_dir,
                           const glm::vec3& half_extents,
                           const glm::vec3& color)
{
    auto safe_normalize = [](const glm::vec3& v) {
        const float len_sq = glm::dot(v, v);
        if (len_sq < 1e-6f)
        {
            return glm::vec3(0.0f, 1.0f, 0.0f);
        }
        return v / std::sqrt(len_sq);
    };

    const glm::vec3 axes[3] = {
        safe_normalize(right_dir),
        safe_normalize(up_dir),
        safe_normalize(forward_dir)};

    std::array<glm::vec3, 8> corners{};
    for (int i = 0; i < 8; ++i)
    {
        const float sx = (i & 1) ? 1.0f : -1.0f;
        const float sy = (i & 2) ? 1.0f : -1.0f;
        const float sz = (i & 4) ? 1.0f : -1.0f;
        corners[i] = center + axes[0] * (sx * half_extents.x) + axes[1] * (sy * half_extents.y) +
                     axes[2] * (sz * half_extents.z);
    }

    const std::array<std::array<int, 4>, 6> faces = {{
        {{0, 1, 3, 2}}, // bottom
        {{4, 5, 7, 6}}, // top
        {{0, 1, 5, 4}}, // front
        {{2, 3, 7, 6}}, // back
        {{1, 3, 7, 5}}, // right
        {{0, 2, 6, 4}}, // left
    }};

    for (const auto& face : faces)
    {
        const std::size_t offset = vertices.size();
        vertices.push_back({corners[face[0]], color});
        vertices.push_back({corners[face[1]], color});
        vertices.push_back({corners[face[2]], color});
        vertices.push_back({corners[face[3]], color});

        indices.push_back(static_cast<unsigned int>(offset + 0));
        indices.push_back(static_cast<unsigned int>(offset + 1));
        indices.push_back(static_cast<unsigned int>(offset + 2));
        indices.push_back(static_cast<unsigned int>(offset + 2));
        indices.push_back(static_cast<unsigned int>(offset + 3));
        indices.push_back(static_cast<unsigned int>(offset + 0));
    }
}

void append_pyramid(std::vector<Vertex>& vertices,
                    std::vector<unsigned int>& indices,
                    const glm::vec3& center,
                    float base_size,
                    float height,
                    const glm::vec3& color)
{
    const float half = base_size * 0.5f;
    const glm::vec3 base_y(center.x, center.y, center.z);
    const std::array<glm::vec3, 5> points = {{
        base_y + glm::vec3(-half, 0.0f, -half),
        base_y + glm::vec3(half, 0.0f, -half),
        base_y + glm::vec3(half, 0.0f, half),
        base_y + glm::vec3(-half, 0.0f, half),
        base_y + glm::vec3(0.0f, height, 0.0f),
    }};

    const std::size_t offset = vertices.size();
    for (const glm::vec3& point : points)
    {
        vertices.push_back({point, color});
    }

    indices.push_back(static_cast<unsigned int>(offset + 0));
    indices.push_back(static_cast<unsigned int>(offset + 1));
    indices.push_back(static_cast<unsigned int>(offset + 2));
    indices.push_back(static_cast<unsigned int>(offset + 2));
    indices.push_back(static_cast<unsigned int>(offset + 3));
    indices.push_back(static_cast<unsigned int>(offset + 0));

    indices.push_back(static_cast<unsigned int>(offset + 0));
    indices.push_back(static_cast<unsigned int>(offset + 1));
    indices.push_back(static_cast<unsigned int>(offset + 4));

    indices.push_back(static_cast<unsigned int>(offset + 1));
    indices.push_back(static_cast<unsigned int>(offset + 2));
    indices.push_back(static_cast<unsigned int>(offset + 4));

    indices.push_back(static_cast<unsigned int>(offset + 2));
    indices.push_back(static_cast<unsigned int>(offset + 3));
    indices.push_back(static_cast<unsigned int>(offset + 4));

    indices.push_back(static_cast<unsigned int>(offset + 3));
    indices.push_back(static_cast<unsigned int>(offset + 0));
    indices.push_back(static_cast<unsigned int>(offset + 4));
}

namespace
{
    constexpr int g_board_columns = 10;
    constexpr int g_board_rows = 10;
    constexpr float g_tile_size = 4.0f;

    enum class TileKind
    {
        Normal,
        Start,
        Finish,
        LadderBase,
        SnakeHead
    };

    struct BoardLink
    {
        int start = 0;
        int end = 0;
        glm::vec3 color{};
        bool is_ladder = false;
    };

    constexpr std::array<BoardLink, 6> g_board_links = {{
        {2, 21, {0.35f, 0.75f, 0.38f}, true},
        {6, 17, {0.32f, 0.68f, 0.82f}, true},
        {20, 38, {0.46f, 0.78f, 0.36f}, true},
        {41, 62, {0.28f, 0.7f, 0.55f}, true},
        {16, 3, {0.78f, 0.24f, 0.24f}, false},
        {47, 25, {0.82f, 0.3f, 0.18f}, false},
    }};

    constexpr int g_digit_rows = 5;
    constexpr int g_digit_cols = 3;
    using DigitGlyph = std::array<uint8_t, g_digit_rows>;
    constexpr std::array<DigitGlyph, 10> g_digit_glyphs = {{
        DigitGlyph{0b111, 0b101, 0b101, 0b101, 0b111}, // 0
        DigitGlyph{0b010, 0b110, 0b010, 0b010, 0b111}, // 1
        DigitGlyph{0b111, 0b001, 0b111, 0b100, 0b111}, // 2
        DigitGlyph{0b111, 0b001, 0b111, 0b001, 0b111}, // 3
        DigitGlyph{0b101, 0b101, 0b111, 0b001, 0b001}, // 4
        DigitGlyph{0b111, 0b100, 0b111, 0b001, 0b111}, // 5
        DigitGlyph{0b111, 0b100, 0b111, 0b101, 0b111}, // 6
        DigitGlyph{0b111, 0b001, 0b010, 0b010, 0b010}, // 7
        DigitGlyph{0b111, 0b101, 0b111, 0b101, 0b111}, // 8
        DigitGlyph{0b111, 0b101, 0b111, 0b001, 0b111}, // 9
    }};

    enum class ActivityKind
    {
        None,
        Bonus,
        Slide,
        Portal,
        Trap
    };
}

glm::vec3 tile_center_world(int tile_index, float height_offset = 0.0f)
{
    const int row = tile_index / g_board_columns;
    const int column_in_row = tile_index % g_board_columns;
    const int column = (row % 2 == 0) ? column_in_row : (g_board_columns - 1 - column_in_row);

    const float board_width = static_cast<float>(g_board_columns) * g_tile_size;
    const float board_height = static_cast<float>(g_board_rows) * g_tile_size;
    const float start_x = -board_width * 0.5f + g_tile_size * 0.5f;
    const float start_z = -board_height * 0.5f + g_tile_size * 0.5f;

    const float x = start_x + static_cast<float>(column) * g_tile_size;
    const float z = start_z + static_cast<float>(row) * g_tile_size;
    return {x, height_offset, z};
}

void append_digit_glyph(std::vector<Vertex>& vertices,
                        std::vector<unsigned int>& indices,
                        int digit,
                        const glm::vec3& center,
                        float cell_size,
                        const glm::vec3& color)
{
    if (digit < 0 || digit > 9)
    {
        return;
    }

    const auto& glyph = g_digit_glyphs[digit];
    const float total_width = static_cast<float>(g_digit_cols) * cell_size;
    const float total_height = static_cast<float>(g_digit_rows) * cell_size;
    const float origin_x = center.x - total_width * 0.5f + cell_size * 0.5f;
    const float origin_z = center.z - total_height * 0.5f + cell_size * 0.5f;
    const float patch_size = cell_size * 0.75f;

    for (int row = 0; row < g_digit_rows; ++row)
    {
        for (int col = 0; col < g_digit_cols; ++col)
        {
            const bool filled = ((glyph[row] >> (g_digit_cols - 1 - col)) & 1U) != 0U;
            if (!filled)
            {
                continue;
            }

            glm::vec3 patch_center(origin_x + static_cast<float>(col) * cell_size,
                                   center.y,
                                   origin_z + static_cast<float>(row) * cell_size);

            const auto [patch_verts, patch_indices] = build_plane(patch_size, patch_size, color, color);
            const std::size_t vertex_offset = vertices.size();
            for (auto vertex : patch_verts)
            {
                vertex.position += patch_center;
                vertices.push_back(vertex);
            }
            for (unsigned int idx : patch_indices)
            {
                indices.push_back(static_cast<unsigned int>(vertex_offset + idx));
            }
        }
    }
}

void append_tile_number(std::vector<Vertex>& vertices,
                        std::vector<unsigned int>& indices,
                        int tile_index,
                        const glm::vec3& tile_center,
                        float tile_size)
{
    const std::string label = std::to_string(tile_index + 1);
    const int digit_count = static_cast<int>(label.size());
    if (digit_count <= 0)
    {
        return;
    }

    const glm::vec3 digit_color = {0.95f, 0.95f, 0.92f};
    const float cell_size = tile_size * 0.05f;
    const float glyph_width = static_cast<float>(g_digit_cols) * cell_size;
    const float glyph_height = static_cast<float>(g_digit_rows) * cell_size;
    const float digit_gap = cell_size * 0.7f;
    const float edge_margin = tile_size * 0.08f;
    const float start_x = tile_center.x - tile_size * 0.5f + edge_margin + glyph_width * 0.5f;
    const float base_z = tile_center.z - tile_size * 0.5f + edge_margin + glyph_height * 0.5f;
    const float base_y = tile_center.y + tile_size * 0.01f;

    for (int i = 0; i < digit_count; ++i)
    {
        const int digit = label[i] - '0';
        glm::vec3 digit_center = tile_center;
        digit_center.x = start_x + static_cast<float>(i) * (glyph_width + digit_gap);
        digit_center.z = base_z;
        digit_center.y = base_y;

        append_digit_glyph(vertices, indices, digit, digit_center, cell_size, digit_color);
    }
}

ActivityKind classify_activity_tile(int tile_index)
{
    const int last_tile = g_board_columns * g_board_rows - 1;
    if (tile_index <= 0 || tile_index >= last_tile)
    {
        return ActivityKind::None;
    }

    if (tile_index % 14 == 0)
    {
        return ActivityKind::Portal;
    }
    if ((tile_index + 5) % 11 == 0)
    {
        return ActivityKind::Slide;
    }
    if ((tile_index % 9) == 0)
    {
        return ActivityKind::Trap;
    }
    if ((tile_index % 4) == 0)
    {
        return ActivityKind::Bonus;
    }

    return ActivityKind::None;
}

void append_activity_icon(std::vector<Vertex>& vertices,
                          std::vector<unsigned int>& indices,
                          ActivityKind kind,
                          const glm::vec3& tile_center,
                          float tile_size)
{
    const glm::vec3 base_center = tile_center + glm::vec3(tile_size * 0.18f, tile_size * 0.02f, -tile_size * 0.18f);

    switch (kind)
    {
    case ActivityKind::Bonus:
    {
        const glm::vec3 gold_base_color(0.92f, 0.76f, 0.18f);
        append_box_prism(vertices,
                         indices,
                         base_center.x,
                         base_center.z,
                         tile_size * 0.2f,
                         tile_size * 0.2f,
                         tile_size * 0.18f,
                         gold_base_color);
        append_box_prism(vertices,
                         indices,
                         base_center.x,
                         base_center.z,
                         tile_size * 0.14f,
                         tile_size * 0.14f,
                         tile_size * 0.28f,
                         {1.0f, 0.92f, 0.44f});
        break;
    }
    case ActivityKind::Trap:
    {
        const glm::vec3 trap_center = tile_center + glm::vec3(-tile_size * 0.15f, tile_center.y, tile_size * 0.15f);
        append_pyramid(vertices,
                       indices,
                       trap_center,
                       tile_size * 0.28f,
                       tile_size * 0.4f,
                       {0.86f, 0.34f, 0.26f});
        break;
    }
    case ActivityKind::Portal:
    {
        const float radius = tile_size * 0.18f;
        const auto [orb_vertices, orb_indices] = build_sphere(radius, 18, 12, {0.3f, 0.78f, 0.95f});
        const std::size_t offset = vertices.size();
        const glm::vec3 orb_center = tile_center + glm::vec3(0.0f, tile_size * 0.18f, 0.0f);
        for (auto vertex : orb_vertices)
        {
            vertex.position += orb_center;
            vertices.push_back(vertex);
        }
        for (unsigned int idx : orb_indices)
        {
            indices.push_back(static_cast<unsigned int>(offset + idx));
        }
        break;
    }
    case ActivityKind::Slide:
    {
        const glm::vec3 ramp_center = tile_center + glm::vec3(tile_size * 0.2f, 0.0f, tile_size * 0.1f);
        const auto [ramp_vertices, ramp_indices] =
            build_plane(tile_size * 0.5f, tile_size * 0.18f, {0.4f, 0.65f, 0.9f}, {0.35f, 0.7f, 0.95f});
        const std::size_t offset = vertices.size();
        for (auto vertex : ramp_vertices)
        {
            vertex.position += ramp_center;
            vertices.push_back(vertex);
        }
        for (unsigned int idx : ramp_indices)
        {
            indices.push_back(static_cast<unsigned int>(offset + idx));
        }

        append_pyramid(vertices,
                       indices,
                       ramp_center + glm::vec3(tile_size * 0.28f, tile_size * 0.02f, 0.0f),
                       tile_size * 0.18f,
                       tile_size * 0.18f,
                       {0.95f, 0.85f, 0.35f});
        break;
    }
    case ActivityKind::None:
    default:
        break;
    }
}

void append_ladder_between_tiles(std::vector<Vertex>& vertices,
                                 std::vector<unsigned int>& indices,
                                 const BoardLink& link,
                                 float surface_height)
{
    glm::vec3 start = tile_center_world(link.start, surface_height);
    glm::vec3 end = tile_center_world(link.end, surface_height);
    glm::vec3 forward = end - start;
    const float span = glm::length(forward);
    if (span < 1e-3f)
    {
        return;
    }
    forward /= span;
    const glm::vec3 up(0.0f, 1.0f, 0.0f);
    glm::vec3 right = glm::cross(up, forward);
    if (glm::dot(right, right) < 1e-6f)
    {
        right = glm::vec3(1.0f, 0.0f, 0.0f);
    }
    right = glm::normalize(right);
    const glm::vec3 mid = 0.5f * (start + end);

    const float rail_spacing = g_tile_size * 0.35f;
    const float rail_width = g_tile_size * 0.04f;
    const float rail_height = g_tile_size * 0.08f;
    const glm::vec3 rail_half_extents(rail_width, rail_height, span * 0.5f);
    const glm::vec3 rail_color = glm::mix(link.color, glm::vec3(0.95f, 0.95f, 0.95f), 0.35f);

    append_oriented_prism(vertices,
                          indices,
                          mid + right * rail_spacing,
                          right,
                          up,
                          forward,
                          rail_half_extents,
                          rail_color);
    append_oriented_prism(vertices,
                          indices,
                          mid - right * rail_spacing,
                          right,
                          up,
                          forward,
                          rail_half_extents,
                          rail_color);

    const int rung_count = std::max(3, static_cast<int>(span / (g_tile_size * 0.4f)));
    const glm::vec3 rung_half_extents(rail_spacing * 0.95f, rail_height * 0.5f, rail_width * 0.6f);
    for (int i = 0; i < rung_count; ++i)
    {
        const float t = static_cast<float>(i) / static_cast<float>(rung_count - 1);
        glm::vec3 rung_center = start + forward * (t * span);
        rung_center.y += rail_height * 0.25f;
        append_oriented_prism(vertices,
                              indices,
                              rung_center,
                              right,
                              up,
                              forward,
                              rung_half_extents,
                              glm::vec3(0.92f, 0.8f, 0.45f));
    }
}

void append_snake_between_tiles(std::vector<Vertex>& vertices,
                                std::vector<unsigned int>& indices,
                                const BoardLink& link,
                                float surface_height)
{
    glm::vec3 start = tile_center_world(link.start, surface_height + g_tile_size * 0.08f);
    glm::vec3 end = tile_center_world(link.end, surface_height + g_tile_size * 0.08f);
    const glm::vec3 delta = end - start;
    const float span = glm::length(delta);
    if (span < 1e-3f)
    {
        return;
    }

    const int segments = 12;
    const float body_radius = g_tile_size * 0.22f;
    const auto [body_vertices, body_indices] = build_sphere(body_radius, 14, 10, link.color);

    for (int i = 0; i < segments; ++i)
    {
        const float t = static_cast<float>(i) / static_cast<float>(segments - 1);
        glm::vec3 position = glm::mix(start, end, t);
        position.y += std::sin(t * glm::pi<float>()) * body_radius * 0.35f;

        const std::size_t offset = vertices.size();
        for (auto vertex : body_vertices)
        {
            vertex.position += position;
            vertices.push_back(vertex);
        }
        for (unsigned int idx : body_indices)
        {
            indices.push_back(static_cast<unsigned int>(offset + idx));
        }
    }

    const auto [head_vertices, head_indices] = build_sphere(body_radius * 1.2f, 16, 12, link.color * 0.9f);
    const std::size_t head_offset = vertices.size();
    for (auto vertex : head_vertices)
    {
        vertex.position += start + glm::vec3(0.0f, body_radius * 0.6f, 0.0f);
        vertices.push_back(vertex);
    }
    for (unsigned int idx : head_indices)
    {
        indices.push_back(static_cast<unsigned int>(head_offset + idx));
    }
}
bool check_wall_collision(const glm::vec3& position, float radius)
{
    const float half_width = static_cast<float>(g_board_columns) * g_tile_size * 0.5f;
    const float half_height = static_cast<float>(g_board_rows) * g_tile_size * 0.5f;

    if (position.x - radius < -half_width || position.x + radius > half_width)
    {
        return true;
    }
    if (position.z - radius < -half_height || position.z + radius > half_height)
    {
        return true;
    }

    return false;
}

std::pair<std::vector<Vertex>, std::vector<unsigned int>> build_snakes_ladders_map()
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    
    const float board_width = static_cast<float>(g_board_columns) * g_tile_size;
    const float board_height = static_cast<float>(g_board_rows) * g_tile_size;
    const float plaza_margin = g_tile_size * 3.0f;
    const float board_margin = g_tile_size * 0.5f;

    const auto push_plane = [&](const std::vector<Vertex>& plane_vertices,
                                const std::vector<unsigned int>& plane_indices,
                                float y_offset) {
        const std::size_t vertex_offset = vertices.size();
        for (const auto& v : plane_vertices)
        {
            Vertex translated = v;
            translated.position.y += y_offset;
            vertices.push_back(translated);
        }
        for (unsigned int idx : plane_indices)
        {
            indices.push_back(static_cast<unsigned int>(vertex_offset + idx));
        }
    };

    const glm::vec3 base_color = {0.08f, 0.07f, 0.07f};
    const auto [plaza_verts, plaza_indices] =
        build_plane(board_width + plaza_margin, board_height + plaza_margin, base_color, base_color);
    push_plane(plaza_verts, plaza_indices, -0.02f);

    const glm::vec3 terrace_color = {0.16f, 0.13f, 0.1f};
    const auto [terrace_verts, terrace_indices] =
        build_plane(board_width + board_margin * 2.0f, board_height + board_margin * 2.0f, terrace_color, terrace_color);
    push_plane(terrace_verts, terrace_indices, 0.0f);

    const glm::vec3 board_plate_color = {0.12f, 0.16f, 0.22f};
    const auto [board_plate_verts, board_plate_indices] = build_plane(board_width, board_height, board_plate_color, board_plate_color);
    push_plane(board_plate_verts, board_plate_indices, 0.015f);

    const float wall_thickness = g_tile_size * 0.45f;
    const float wall_height = 2.4f;
    const glm::vec3 wall_color = {0.15f, 0.2f, 0.32f};
    const float half_board_w = board_width * 0.5f;
    const float half_board_h = board_height * 0.5f;

    append_box_prism(vertices,
                     indices,
                     0.0f,
                     half_board_h + wall_thickness * 0.5f,
                     board_width + wall_thickness * 2.0f,
                     wall_thickness,
                     wall_height,
                     wall_color);
    append_box_prism(vertices,
                     indices,
                     0.0f,
                     -(half_board_h + wall_thickness * 0.5f),
                     board_width + wall_thickness * 2.0f,
                     wall_thickness,
                     wall_height,
                     wall_color);
    append_box_prism(vertices,
                     indices,
                     half_board_w + wall_thickness * 0.5f,
                     0.0f,
                     wall_thickness,
                     board_height + wall_thickness * 2.0f,
                     wall_height,
                     wall_color);
    append_box_prism(vertices,
                     indices,
                     -(half_board_w + wall_thickness * 0.5f),
                     0.0f,
                     wall_thickness,
                     board_height + wall_thickness * 2.0f,
                     wall_height,
                     wall_color);

    const glm::vec3 pillar_color = {0.22f, 0.22f, 0.3f};
    const float pillar_size = wall_thickness * 0.85f;
    const float pillar_height = wall_height * 1.15f;
    const float pillar_offset_x = half_board_w + wall_thickness * 0.5f;
    const float pillar_offset_z = half_board_h + wall_thickness * 0.5f;
    const std::array<glm::vec2, 4> pillar_offsets = {{
        {pillar_offset_x, pillar_offset_z},
        {-pillar_offset_x, pillar_offset_z},
        {pillar_offset_x, -pillar_offset_z},
        {-pillar_offset_x, -pillar_offset_z},
    }};
    for (const auto& offset : pillar_offsets)
    {
        append_box_prism(vertices,
                         indices,
                         offset.x,
                         offset.y,
                         pillar_size,
                         pillar_size,
                         pillar_height,
                         pillar_color);
    }

    std::array<TileKind, g_board_columns * g_board_rows> tile_kinds{};
    tile_kinds.fill(TileKind::Normal);
    tile_kinds.front() = TileKind::Start;
    tile_kinds.back() = TileKind::Finish;

    for (const auto& link : g_board_links)
    {
        tile_kinds[link.start] = link.is_ladder ? TileKind::LadderBase : TileKind::SnakeHead;
    }

    const glm::vec3 color_a = {0.18f, 0.28f, 0.45f};
    const glm::vec3 color_b = {0.16f, 0.24f, 0.4f};
    const glm::vec3 start_color = {0.22f, 0.65f, 0.28f};
    const glm::vec3 finish_color = {0.85f, 0.63f, 0.22f};
    const glm::vec3 ladder_color = {0.35f, 0.7f, 0.4f};
    const glm::vec3 snake_color = {0.78f, 0.28f, 0.28f};

    const float tile_surface_offset = 0.02f;
    const float tile_size = g_tile_size * 0.92f;

    for (int tile = 0; tile < g_board_columns * g_board_rows; ++tile)
    {
        glm::vec3 color = ((tile / g_board_columns + tile % g_board_columns) % 2 == 0) ? color_a : color_b;

        switch (tile_kinds[tile])
        {
        case TileKind::Start:
            color = start_color;
            break;
        case TileKind::Finish:
            color = finish_color;
            break;
        case TileKind::LadderBase:
            color = ladder_color;
            break;
        case TileKind::SnakeHead:
            color = snake_color;
            break;
        default:
            break;
        }

        const auto [tile_verts, tile_indices] = build_plane(tile_size, tile_size, color, color);
        const std::size_t tile_offset = vertices.size();
        const glm::vec3 center = tile_center_world(tile, tile_surface_offset);
        for (const auto& v : tile_verts)
        {
            Vertex translated = v;
            translated.position += center;
            vertices.push_back(translated);
        }
        for (unsigned int idx : tile_indices)
        {
            indices.push_back(static_cast<unsigned int>(tile_offset + idx));
        }

        append_tile_number(vertices, indices, tile, center, tile_size);
        const ActivityKind activity = classify_activity_tile(tile);
        if (activity != ActivityKind::None)
        {
            append_activity_icon(vertices, indices, activity, center, tile_size);
        }
    }

    for (const auto& link : g_board_links)
    {
        if (link.is_ladder)
        {
            append_ladder_between_tiles(vertices, indices, link, tile_surface_offset + 0.05f);
        }
        else
        {
            append_snake_between_tiles(vertices, indices, link, tile_surface_offset + 0.05f);
        }
    }

    const float marker_radius = 0.35f;
    const auto [marker_verts, marker_indices] = build_sphere(marker_radius, 12, 6, {1.0f, 1.0f, 1.0f});

    for (const auto& link : g_board_links)
    {
        const float elevation = marker_radius + tile_surface_offset;
        const glm::vec3 start_center = tile_center_world(link.start, elevation);
        const glm::vec3 end_center = tile_center_world(link.end, elevation);

        const std::size_t start_marker_offset = vertices.size();
        for (const auto& v : marker_verts)
        {
            Vertex translated = v;
            translated.position += start_center;
            translated.color = link.color;
            vertices.push_back(translated);
        }
        for (unsigned int idx : marker_indices)
        {
            indices.push_back(static_cast<unsigned int>(start_marker_offset + idx));
        }

        const std::size_t end_marker_offset = vertices.size();
        for (const auto& v : marker_verts)
        {
            Vertex translated = v;
            translated.position += end_center;
            translated.color = link.color;
            vertices.push_back(translated);
        }
        for (unsigned int idx : marker_indices)
        {
            indices.push_back(static_cast<unsigned int>(end_marker_offset + idx));
        }
    }
    
    return {vertices, indices};
}

int main(int argc, char* argv[])
{
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW\n";
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if defined(__APPLE__)
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(800, 600, "Pacman OpenGL", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSwapInterval(1);

    if (gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)) == 0)
    {
        std::cerr << "Failed to initialize GLAD\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    std::filesystem::path executable_dir = std::filesystem::current_path();
    if (argc > 0)
    {
        std::error_code ec;
        auto resolved = std::filesystem::weakly_canonical(std::filesystem::path(argv[0]), ec);
        if (!ec)
        {
            executable_dir = resolved.parent_path();
        }
    }

    const auto shaders_dir = executable_dir / "shaders";
    try
    {
        const std::string vertex_source = load_file(shaders_dir / "simple.vert");
        const std::string fragment_source = load_file(shaders_dir / "simple.frag");

        GLuint program = create_program(vertex_source, fragment_source);
        const GLint mvp_location = glGetUniformLocation(program, "uMVP");

        glEnable(GL_DEPTH_TEST);

        Mesh map_mesh{};
        Bounds map_bounds{};
        
        const auto [map_vertices, map_indices] = build_snakes_ladders_map();
        map_mesh = create_mesh(map_vertices, map_indices);
        
        const float board_width = static_cast<float>(g_board_columns) * g_tile_size;
        const float board_height = static_cast<float>(g_board_rows) * g_tile_size;
        map_bounds.min = glm::vec3(-board_width * 0.5f, 0.0f, -board_height * 0.5f);
        map_bounds.max = glm::vec3(board_width * 0.5f, 0.0f, board_height * 0.5f);

        const float map_min_dimension = std::min(board_width, board_height);
        const float map_length = board_height;

        const float player_radius = std::max(0.4f, 0.025f * map_min_dimension);
        const auto [sphere_vertices, sphere_indices] = build_sphere(player_radius, 32, 16, {1.0f, 0.9f, 0.1f});
        Mesh sphere_mesh = create_mesh(sphere_vertices, sphere_indices);

        const float dice_size = g_tile_size * 0.9f;
        const auto [dice_vertices, dice_indices] = build_cube(dice_size, {0.95f, 0.95f, 0.92f});
        Mesh dice_mesh = create_mesh(dice_vertices, dice_indices);
        const auto [pip_vertices, pip_indices] = build_sphere(dice_size * 0.1f, 12, 6, {0.08f, 0.08f, 0.08f});
        Mesh pip_mesh = create_mesh(pip_vertices, pip_indices);
        std::array<Mesh, 10> digit_meshes{};
        for (int digit = 0; digit <= 9; ++digit)
        {
            std::vector<Vertex> digit_vertices;
            std::vector<unsigned int> digit_indices;
            append_digit_glyph(digit_vertices,
                               digit_indices,
                               digit,
                               glm::vec3(0.0f),
                               dice_size * 0.07f,
                               glm::vec3(1.0f));
            digit_meshes[static_cast<std::size_t>(digit)] = create_mesh(digit_vertices, digit_indices);
        }
        glm::vec3 dice_position(map_bounds.min.x - dice_size * 0.8f,
                                map_bounds.min.y + dice_size * 0.5f,
                                map_bounds.min.z - dice_size * 0.8f);
        glm::vec3 dice_velocity(0.0f);
        float dice_target_height = dice_position.y;
        const std::array<std::vector<glm::vec2>, 7> pip_layouts = {
            std::vector<glm::vec2>{},
            std::vector<glm::vec2>{glm::vec2(0.0f, 0.0f)},
            std::vector<glm::vec2>{glm::vec2(-0.35f, -0.35f), glm::vec2(0.35f, 0.35f)},
            std::vector<glm::vec2>{glm::vec2(-0.35f, -0.35f), glm::vec2(0.0f, 0.0f), glm::vec2(0.35f, 0.35f)},
            std::vector<glm::vec2>{glm::vec2(-0.35f, -0.35f), glm::vec2(-0.35f, 0.35f),
                                   glm::vec2(0.35f, -0.35f), glm::vec2(0.35f, 0.35f)},
            std::vector<glm::vec2>{glm::vec2(-0.35f, -0.35f), glm::vec2(-0.35f, 0.35f),
                                   glm::vec2(0.35f, -0.35f), glm::vec2(0.35f, 0.35f), glm::vec2(0.0f, 0.0f)},
            std::vector<glm::vec2>{glm::vec2(-0.35f, -0.4f), glm::vec2(-0.35f, 0.0f), glm::vec2(-0.35f, 0.4f),
                                   glm::vec2(0.35f, -0.4f), glm::vec2(0.35f, 0.0f), glm::vec2(0.35f, 0.4f)}};

        const float player_ground_y = map_bounds.min.y + player_radius;
        glm::vec3 player_position = tile_center_world(0);
        player_position.y = player_ground_y;
        const int final_tile_index = g_board_columns * g_board_rows - 1;
        int current_tile_index = 0;
        bool is_stepping = false;
        glm::vec3 step_start_position = player_position;
        glm::vec3 step_end_position = player_position;
        float step_timer = 0.0f;
        const float step_duration = 0.55f;
        bool previous_space_state = false;
        int steps_remaining = 0;
        int last_dice_result = 0;
        std::mt19937 rng(static_cast<unsigned int>(
            std::chrono::steady_clock::now().time_since_epoch().count()));
        std::uniform_int_distribution<int> dice_distribution(1, 6);
        struct DiceFace
        {
            glm::vec3 normal;
            glm::vec3 tangent;
            glm::vec3 bitangent;
            glm::vec3 center_offset;
            int value;
        };
        const std::array<DiceFace, 6> dice_faces = {{
            {{0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, dice_size * 0.5f, 0.0f}, 1},
            {{0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, -dice_size * 0.5f, 0.0f}, 6},
            {{0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, dice_size * 0.5f}, 2},
            {{0.0f, 0.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, -dice_size * 0.5f}, 5},
            {{1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {dice_size * 0.5f, 0.0f, 0.0f}, 3},
            {{-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {-dice_size * 0.5f, 0.0f, 0.0f}, 4},
        }};

        auto schedule_step = [&]() {
            if (steps_remaining <= 0 || is_stepping || current_tile_index >= final_tile_index)
            {
                return;
            }

            step_start_position = tile_center_world(current_tile_index);
            step_start_position.y = player_ground_y;

            step_end_position = tile_center_world(current_tile_index + 1);
            step_end_position.y = player_ground_y;

            is_stepping = true;
            step_timer = 0.0f;
        };

        float last_time = static_cast<float>(glfwGetTime());

        while (!glfwWindowShouldClose(window))
        {
            const float current_time = static_cast<float>(glfwGetTime());
            const float delta_time = current_time - last_time;
            last_time = current_time;

            glfwPollEvents();

            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }

            const bool space_pressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
            const bool space_just_pressed = space_pressed && !previous_space_state;

            if (space_just_pressed && !is_stepping && current_tile_index < final_tile_index)
            {
                last_dice_result = dice_distribution(rng);
                steps_remaining = std::min(last_dice_result, final_tile_index - current_tile_index);
                if (steps_remaining > 0)
                {
                    schedule_step();
                    dice_velocity = glm::vec3(0.0f, dice_size * 1.8f, 0.0f);
                    dice_target_height = dice_position.y + dice_size * 0.6f;
                }
            }

            dice_velocity.y -= 9.8f * delta_time;
            dice_position += dice_velocity * delta_time;
            if (dice_position.y <= dice_target_height)
            {
                dice_position.y = dice_target_height;
                dice_velocity = glm::vec3(0.0f);
                dice_target_height = map_bounds.min.y + dice_size * 0.5f;
            }

            if (is_stepping)
            {
                step_timer += delta_time;
                const float t = glm::clamp(step_timer / step_duration, 0.0f, 1.0f);
                const float ease = t * t * (3.0f - 2.0f * t);
                glm::vec3 interpolated = glm::mix(step_start_position, step_end_position, ease);
                interpolated.y += std::sin(ease * glm::pi<float>()) * player_radius * 0.18f;
                player_position = interpolated;

                if (t >= 1.0f - 1e-4f)
                {
                    player_position = step_end_position;
                    player_position.y = player_ground_y;
                    ++current_tile_index;
                    --steps_remaining;
                    is_stepping = false;
                    if (steps_remaining > 0 && current_tile_index < final_tile_index)
                    {
                        schedule_step();
                    }
                }
            }
            else
            {
                player_position.y = player_ground_y;
            }

            previous_space_state = space_pressed;

            glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            int framebuffer_width = 0;
            int framebuffer_height = 0;
            glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
            const float aspect_ratio =
                static_cast<float>(framebuffer_width > 0 ? framebuffer_width : 1) /
                static_cast<float>(framebuffer_height > 0 ? framebuffer_height : 1);

            const glm::mat4 projection = glm::perspective(glm::radians(g_camera_fov),
                                                          aspect_ratio,
                                                          0.1f,
                                                          5000.0f);

            const float camera_height = std::max(12.0f, map_length * 0.35f);
            const float camera_distance = std::max(18.0f, map_length * 0.55f);
            const glm::vec3 camera_offset(0.0f, camera_height, camera_distance);
            const glm::vec3 camera_position = player_position + camera_offset;
            const glm::mat4 view = glm::lookAt(camera_position,
                                               player_position + glm::vec3(0.0f, 0.0f, -camera_distance * 0.35f),
                                               glm::vec3(0.0f, 1.0f, 0.0f));

            glUseProgram(program);

            {
                const glm::mat4 model(1.0f);
                const glm::mat4 mvp = projection * view * model;
                glUniformMatrix4fv(mvp_location, 1, GL_FALSE, glm::value_ptr(mvp));

                glBindVertexArray(map_mesh.vao);
                glDrawElements(GL_TRIANGLES, map_mesh.index_count, GL_UNSIGNED_INT, nullptr);
            }

            {
                const glm::mat4 model = glm::translate(glm::mat4(1.0f), player_position);
                const glm::mat4 mvp = projection * view * model;
                glUniformMatrix4fv(mvp_location, 1, GL_FALSE, glm::value_ptr(mvp));

                glBindVertexArray(sphere_mesh.vao);
                glDrawElements(GL_TRIANGLES, sphere_mesh.index_count, GL_UNSIGNED_INT, nullptr);
            }

            {
                const glm::mat4 model = glm::translate(glm::mat4(1.0f), dice_position);
                const glm::mat4 mvp = projection * view * model;
                glUniformMatrix4fv(mvp_location, 1, GL_FALSE, glm::value_ptr(mvp));

                glBindVertexArray(dice_mesh.vao);
                glDrawElements(GL_TRIANGLES, dice_mesh.index_count, GL_UNSIGNED_INT, nullptr);

                const float pip_scale = dice_size * 0.4f;
                const float pip_offset = dice_size * 0.05f;
                for (const auto& face : dice_faces)
                {
                    const auto& layout = pip_layouts[face.value];
                    for (const auto& offset : layout)
                    {
                        glm::vec3 pip_pos = dice_position + face.center_offset +
                                            face.tangent * (offset.x * pip_scale) +
                                            face.bitangent * (offset.y * pip_scale) +
                                            face.normal * pip_offset;
                        const glm::mat4 pip_model = glm::translate(glm::mat4(1.0f), pip_pos);
                        const glm::mat4 pip_mvp = projection * view * pip_model;
                        glUniformMatrix4fv(mvp_location, 1, GL_FALSE, glm::value_ptr(pip_mvp));
                        glBindVertexArray(pip_mesh.vao);
                        glDrawElements(GL_TRIANGLES, pip_mesh.index_count, GL_UNSIGNED_INT, nullptr);
                    }
                }

                if (last_dice_result >= 1 && last_dice_result <= 6)
                {
                    const float text_height = dice_size * 0.95f;
                    const Mesh& digit_mesh = digit_meshes[static_cast<std::size_t>(last_dice_result)];
                    const glm::mat4 digit_model =
                        glm::translate(glm::mat4(1.0f), dice_position + glm::vec3(0.0f, text_height, 0.0f));
                    const glm::mat4 digit_mvp = projection * view * digit_model;
                    glUniformMatrix4fv(mvp_location, 1, GL_FALSE, glm::value_ptr(digit_mvp));
                    glBindVertexArray(digit_mesh.vao);
                    glDrawElements(GL_TRIANGLES, digit_mesh.index_count, GL_UNSIGNED_INT, nullptr);
                }
            }


            glfwSwapBuffers(window);
        }

        destroy_mesh(map_mesh);
        destroy_mesh(sphere_mesh);
        destroy_mesh(dice_mesh);
        destroy_mesh(pip_mesh);
        for (auto& mesh : digit_meshes)
        {
            destroy_mesh(mesh);
        }
        glDeleteProgram(program);
    }
    catch (const std::exception& ex)
    {
        std::cerr << ex.what() << '\n';
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

