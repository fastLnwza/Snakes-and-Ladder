#pragma once

#include "core/types.h"
#include "texture_loader.h"
#include <filesystem>
#include <vector>

struct GLTFModel
{
    std::vector<Mesh> meshes;
    glm::mat4 base_transform{1.0f};
    std::vector<Texture> textures;  // Textures loaded from the model
};

GLTFModel load_gltf_model(const std::filesystem::path& path);
void destroy_gltf_model(GLTFModel& model);



