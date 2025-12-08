#pragma once

#include "core/types.h"
#include "texture_loader.h"
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <filesystem>
#include <vector>
#include <string>
#include <map>

// FBX Model structure (similar to GLTF but for FBX)
struct FBXModel
{
    std::vector<Mesh> meshes;
    glm::mat4 base_transform{1.0f};
    std::vector<Texture> textures;  // Textures loaded from the model
};

FBXModel load_fbx_model(const std::filesystem::path& path);
void destroy_fbx_model(FBXModel& model);






