#pragma once

#include "core/types.h"
#include <filesystem>
#include <vector>

struct OBJModel
{
    std::vector<Mesh> meshes;
    glm::mat4 base_transform{1.0f};
};

OBJModel load_obj_model(const std::filesystem::path& path);
void destroy_obj_model(OBJModel& model);



