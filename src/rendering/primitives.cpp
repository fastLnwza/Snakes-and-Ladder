#include "primitives.h"

#include <array>
#include <cmath>
#include <glm/gtc/constants.hpp>

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

