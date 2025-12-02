#pragma once

#include "core/types.h"
#include <filesystem>
#include <vector>

struct GLTFModel
{
    std::vector<Mesh> meshes;
    glm::mat4 base_transform{1.0f};
};

GLTFModel load_gltf_model(const std::filesystem::path& path);
void destroy_gltf_model(GLTFModel& model);



