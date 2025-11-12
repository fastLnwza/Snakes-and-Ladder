#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
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
        const auto map_path = executable_dir / "pac_man_map_moderno" / "scene.gltf";
        constexpr float target_map_extent = 160.0f;
        if (!load_gltf_model(map_path, map_mesh, map_bounds, {0.35f, 0.35f, 0.38f}, target_map_extent))
        {
            throw std::runtime_error("Failed to load map model: " + map_path.string());
        }

        const float map_width = map_bounds.max.x - map_bounds.min.x;
        const float map_length = map_bounds.max.z - map_bounds.min.z;
        const float map_min_dimension = std::min(map_width, map_length);

        const float player_radius = std::max(0.75f, 0.04f * map_min_dimension);
        const auto [sphere_vertices, sphere_indices] = build_sphere(player_radius, 32, 16, {1.0f, 0.9f, 0.1f});
        Mesh sphere_mesh = create_mesh(sphere_vertices, sphere_indices);

        const glm::vec3 map_center = 0.5f * (map_bounds.min + map_bounds.max);
        const float player_ground_y = map_bounds.min.y + player_radius;
        glm::vec3 player_position(map_center.x, player_ground_y, map_center.z);
        const float player_speed = std::max(6.0f, player_radius * 4.0f);

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

            glm::vec3 input(0.0f);
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            {
                input.x -= 1.0f;
            }
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            {
                input.x += 1.0f;
            }
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            {
                input.z -= 1.0f;
            }
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            {
                input.z += 1.0f;
            }

            if (glm::dot(input, input) > 0.0f)
            {
                input = glm::normalize(input);
                player_position += input * player_speed * delta_time;
            }

            player_position.x = glm::clamp(player_position.x,
                                           map_bounds.min.x + player_radius,
                                           map_bounds.max.x - player_radius);
            player_position.z = glm::clamp(player_position.z,
                                           map_bounds.min.z + player_radius,
                                           map_bounds.max.z - player_radius);
            player_position.y = player_ground_y;

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

            glfwSwapBuffers(window);
        }

        destroy_mesh(map_mesh);
        destroy_mesh(sphere_mesh);
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

