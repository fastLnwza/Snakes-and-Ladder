#include "map_generator.h"

#include "board.h"
#include "rendering/primitives.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <glm/gtc/constants.hpp>

namespace game::map
{
    namespace
    {
        constexpr int DIGIT_ROWS = 5;
        constexpr int DIGIT_COLS = 3;
        using DigitGlyph = std::array<uint8_t, DIGIT_ROWS>;
        
        constexpr std::array<DigitGlyph, 10> DIGIT_GLYPHS = {{
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

            const auto& glyph = DIGIT_GLYPHS[digit];
            const float total_width = static_cast<float>(DIGIT_COLS) * cell_size;
            const float total_height = static_cast<float>(DIGIT_ROWS) * cell_size;
            const float origin_x = center.x - total_width * 0.5f + cell_size * 0.5f;
            const float origin_z = center.z - total_height * 0.5f + cell_size * 0.5f;
            const float patch_size = cell_size * 0.75f;

            for (int row = 0; row < DIGIT_ROWS; ++row)
            {
                for (int col = 0; col < DIGIT_COLS; ++col)
                {
                    const bool filled = ((glyph[row] >> (DIGIT_COLS - 1 - col)) & 1U) != 0U;
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
            const float glyph_width = static_cast<float>(DIGIT_COLS) * cell_size;
            const float glyph_height = static_cast<float>(DIGIT_ROWS) * cell_size;
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

        void append_start_icon(std::vector<Vertex>& vertices,
                               std::vector<unsigned int>& indices,
                               const glm::vec3& tile_center,
                               float tile_size)
        {
            // Create a green triangle marker for start point
            const float triangle_size = tile_size * 0.25f;
            const float triangle_height = tile_size * 0.3f;
            const glm::vec3 triangle_center = tile_center + glm::vec3(tile_size * 0.18f, tile_size * 0.02f, -tile_size * 0.18f);
            
            append_pyramid(vertices,
                           indices,
                           triangle_center,
                           triangle_size,
                           triangle_height,
                           {0.22f, 0.85f, 0.32f}); // Bright green for start triangle
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
            case ActivityKind::MiniGame:
            {
                const float orb_radius = tile_size * 0.2f;
                const auto [orb_vertices, orb_indices] = build_sphere(orb_radius, 20, 14, {0.72f, 0.35f, 0.92f});
                const std::size_t offset = vertices.size();
                const glm::vec3 orb_center = tile_center + glm::vec3(0.0f, tile_size * 0.22f, 0.0f);
                for (auto vertex : orb_vertices)
                {
                    vertex.position += orb_center;
                    vertices.push_back(vertex);
                }
                for (unsigned int idx : orb_indices)
                {
                    indices.push_back(static_cast<unsigned int>(offset + idx));
                }

                const glm::vec3 ring_color = {0.32f, 0.78f, 0.95f};
                append_box_prism(vertices,
                                 indices,
                                 tile_center.x,
                                 tile_center.z,
                                 tile_size * 0.45f,
                                 tile_size * 0.08f,
                                 tile_size * 0.05f,
                                 ring_color);
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

            const float rail_spacing = TILE_SIZE * 0.35f;
            const float rail_width = TILE_SIZE * 0.04f;
            const float rail_height = TILE_SIZE * 0.08f;
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

            const int rung_count = std::max(3, static_cast<int>(span / (TILE_SIZE * 0.4f)));
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
            glm::vec3 start = tile_center_world(link.start, surface_height + TILE_SIZE * 0.08f);
            glm::vec3 end = tile_center_world(link.end, surface_height + TILE_SIZE * 0.08f);
            const glm::vec3 delta = end - start;
            const float span = glm::length(delta);
            if (span < 1e-3f)
            {
                return;
            }

            const int segments = 12;
            const float body_radius = TILE_SIZE * 0.22f;
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
    }

    std::pair<std::vector<Vertex>, std::vector<unsigned int>> build_snakes_ladders_map()
    {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        
        const float board_width = static_cast<float>(BOARD_COLUMNS) * TILE_SIZE;
        const float board_height = static_cast<float>(BOARD_ROWS) * TILE_SIZE;
        const float plaza_margin = TILE_SIZE * 0.2f;  // Reduced margin so board fills plaza
        const float board_margin = TILE_SIZE * 0.3f;  // Reduced margin

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

        const float wall_thickness = TILE_SIZE * 0.45f;
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

        std::array<TileKind, BOARD_COLUMNS * BOARD_ROWS> tile_kinds{};
        tile_kinds.fill(TileKind::Normal);
        tile_kinds.front() = TileKind::Start;
        tile_kinds.back() = TileKind::Finish;

        for (const auto& link : BOARD_LINKS)
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
        const float tile_size = TILE_SIZE * 0.98f;  // Increased to fill board better

        for (int tile = 0; tile < BOARD_COLUMNS * BOARD_ROWS; ++tile)
        {
            glm::vec3 color = ((tile / BOARD_COLUMNS + tile % BOARD_COLUMNS) % 2 == 0) ? color_a : color_b;

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
            
            // Add start icon for start tile
            if (tile_kinds[tile] == TileKind::Start)
            {
                append_start_icon(vertices, indices, center, tile_size);
            }
            
            const ActivityKind activity = classify_activity_tile(tile);
            if (activity != ActivityKind::None)
            {
                append_activity_icon(vertices, indices, activity, center, tile_size);
            }
        }

        for (const auto& link : BOARD_LINKS)
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

        for (const auto& link : BOARD_LINKS)
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
}

