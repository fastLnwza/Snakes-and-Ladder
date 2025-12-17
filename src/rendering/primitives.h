#pragma once

#include "core/types.h"
#include <glm/glm.hpp>
#include <vector>

std::pair<std::vector<Vertex>, std::vector<unsigned int>> build_plane(float length,
                                                                      float width,
                                                                      const glm::vec3& color_center,
                                                                      const glm::vec3& color_edge);

std::pair<std::vector<Vertex>, std::vector<unsigned int>> build_sphere(float radius,
                                                                       int sector_count,
                                                                       int stack_count,
                                                                       const glm::vec3& color);

void append_box_prism(std::vector<Vertex>& vertices,
                      std::vector<unsigned int>& indices,
                      float center_x,
                      float center_z,
                      float width,
                      float length,
                      float height,
                      const glm::vec3& color);

void append_oriented_prism(std::vector<Vertex>& vertices,
                           std::vector<unsigned int>& indices,
                           const glm::vec3& center,
                           const glm::vec3& right_dir,
                           const glm::vec3& up_dir,
                           const glm::vec3& forward_dir,
                           const glm::vec3& half_extents,
                           const glm::vec3& color);

void append_pyramid(std::vector<Vertex>& vertices,
                    std::vector<unsigned int>& indices,
                    const glm::vec3& center,
                    float base_size,
                    float height,
                    const glm::vec3& color);

