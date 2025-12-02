#pragma once

#include "core/types.h"
#include <vector>

Mesh create_mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);
void destroy_mesh(Mesh& mesh);

