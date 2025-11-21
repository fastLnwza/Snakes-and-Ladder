#include "gltf_loader.h"

#define CGLTF_IMPLEMENTATION
#include <cgltf/cgltf.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stdexcept>

#include "mesh.h"
#include "utils/file_utils.h"

GLTFModel load_gltf_model(const std::filesystem::path& path)
{
    GLTFModel model;

    // Parse GLTF file
    cgltf_options options{};
    cgltf_data* data = nullptr;
    cgltf_result result = cgltf_parse_file(&options, path.string().c_str(), &data);

    if (result != cgltf_result_success)
    {
        throw std::runtime_error("Failed to parse GLTF file: " + path.string());
    }

    // Load buffer data
    result = cgltf_load_buffers(&options, data, path.string().c_str());
    if (result != cgltf_result_success)
    {
        cgltf_free(data);
        throw std::runtime_error("Failed to load GLTF buffers: " + path.string());
    }

    // Process all meshes in the scene
    for (cgltf_size i = 0; i < data->meshes_count; ++i)
    {
        const cgltf_mesh* gltf_mesh = &data->meshes[i];
        
        for (cgltf_size j = 0; j < gltf_mesh->primitives_count; ++j)
        {
            const cgltf_primitive* primitive = &gltf_mesh->primitives[j];

            std::vector<Vertex> vertices;
            std::vector<unsigned int> indices;

            // Extract vertex positions
            cgltf_accessor* position_accessor = nullptr;
            cgltf_accessor* normal_accessor = nullptr;
            cgltf_accessor* color_accessor = nullptr;

            for (cgltf_size k = 0; k < primitive->attributes_count; ++k)
            {
                const cgltf_attribute* attr = &primitive->attributes[k];
                if (attr->type == cgltf_attribute_type_position)
                {
                    position_accessor = attr->data;
                }
                else if (attr->type == cgltf_attribute_type_normal)
                {
                    normal_accessor = attr->data;
                }
                else if (attr->type == cgltf_attribute_type_color)
                {
                    color_accessor = attr->data;
                }
            }

            if (!position_accessor)
            {
                continue; // Skip primitives without positions
            }

            // Extract indices
            cgltf_accessor* index_accessor = primitive->indices;
            cgltf_size vertex_count = position_accessor->count;

            // Unpack positions
            std::vector<float> positions(vertex_count * 3);
            cgltf_accessor_unpack_floats(position_accessor, positions.data(), vertex_count * 3);

            // Unpack normals if available
            std::vector<float> normals;
            if (normal_accessor && normal_accessor->count == vertex_count)
            {
                normals.resize(vertex_count * 3);
                cgltf_accessor_unpack_floats(normal_accessor, normals.data(), vertex_count * 3);
            }

            // Unpack colors if available
            std::vector<float> colors;
            if (color_accessor && color_accessor->count == vertex_count)
            {
                colors.resize(vertex_count * cgltf_num_components(color_accessor->type));
                cgltf_accessor_unpack_floats(color_accessor, colors.data(), 
                    vertex_count * cgltf_num_components(color_accessor->type));
            }

            // Create vertices
            for (cgltf_size v = 0; v < vertex_count; ++v)
            {
                Vertex vertex{};
                vertex.position = glm::vec3(
                    positions[v * 3 + 0],
                    positions[v * 3 + 1],
                    positions[v * 3 + 2]
                );

                // Use color from accessor if available, otherwise use default white
                if (!colors.empty())
                {
                    int num_components = cgltf_num_components(color_accessor->type);
                    vertex.color = glm::vec3(
                        colors[v * num_components + 0],
                        num_components > 1 ? colors[v * num_components + 1] : colors[v * num_components + 0],
                        num_components > 2 ? colors[v * num_components + 2] : colors[v * num_components + 0]
                    );
                }
                else
                {
                    // Default to white color
                    vertex.color = glm::vec3(1.0f, 1.0f, 1.0f);
                }

                vertices.push_back(vertex);
            }

            // Extract indices
            if (index_accessor)
            {
                cgltf_size index_count = index_accessor->count;
                indices.reserve(index_count);

                for (cgltf_size idx = 0; idx < index_count; ++idx)
                {
                    cgltf_size index_value = cgltf_accessor_read_index(index_accessor, idx);
                    indices.push_back(static_cast<unsigned int>(index_value));
                }
            }
            else
            {
                // No indices, create sequential indices
                for (cgltf_size idx = 0; idx < vertex_count; ++idx)
                {
                    indices.push_back(static_cast<unsigned int>(idx));
                }
            }

            // Create mesh from vertices and indices
            if (!vertices.empty() && !indices.empty())
            {
                Mesh mesh = create_mesh(vertices, indices);
                model.meshes.push_back(mesh);
            }
        }
    }

    cgltf_free(data);
    return model;
}

void destroy_gltf_model(GLTFModel& model)
{
    for (auto& mesh : model.meshes)
    {
        destroy_mesh(mesh);
    }
    model.meshes.clear();
}

