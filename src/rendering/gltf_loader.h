#pragma once

#include "core/types.h"
#include "texture_loader.h"
#include <glm/gtc/quaternion.hpp>
#include <filesystem>
#include <vector>
#include <string>

// Animation data structure
struct GLTFAnimationChannel
{
    std::string target_path;  // "translation", "rotation", "scale"
    size_t target_node_index; // Index of the node this channel targets
    std::string target_node_name; // Name of target node (for debugging)
    std::vector<float> keyframe_times;  // Time values for keyframes
    std::vector<glm::vec3> translation_keys;  // Translation keyframes
    std::vector<glm::quat> rotation_keys;     // Rotation keyframes (quaternions)
    std::vector<glm::vec3> scale_keys;        // Scale keyframes
};

struct GLTFAnimation
{
    std::string name;
    float duration;  // Total duration of animation in seconds
    std::vector<GLTFAnimationChannel> channels;
};

struct GLTFModel
{
    std::vector<Mesh> meshes;
    glm::mat4 base_transform{1.0f};
    std::vector<Texture> textures;  // Textures loaded from the model
    std::vector<GLTFAnimation> animations;  // Animations from the model
};

GLTFModel load_gltf_model(const std::filesystem::path& path);
void destroy_gltf_model(GLTFModel& model);



