#include "gltf_loader.h"

#define CGLTF_IMPLEMENTATION
#include <cgltf/cgltf.h>
// Note: STB_IMAGE_IMPLEMENTATION is already defined in texture_loader.cpp
// Just include the header without defining implementation
#include "../../external/stb_image.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stdexcept>
#include <iostream>

#include "mesh.h"
#include "texture_loader.h"
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

    // First, load all textures
    std::vector<Texture> loaded_textures;
    for (cgltf_size img_idx = 0; img_idx < data->images_count; ++img_idx)
    {
        const cgltf_image* image = &data->images[img_idx];
        
        // Check if image data is embedded (GLB format)
        if (image->buffer_view)
        {
            // Image data is embedded in GLB buffer
            const cgltf_buffer_view* view = image->buffer_view;
            const cgltf_buffer* buffer = view->buffer;
            const unsigned char* image_data = static_cast<const unsigned char*>(buffer->data) + view->offset;
            
            int width, height, channels;
            unsigned char* decoded_data = stbi_load_from_memory(
                image_data, 
                static_cast<int>(view->size),
                &width, &height, &channels, 
                0
            );
            
            if (decoded_data)
            {
                Texture texture{};
                texture.width = width;
                texture.height = height;
                
                glGenTextures(1, &texture.id);
                glBindTexture(GL_TEXTURE_2D, texture.id);
                
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                
                GLenum format = GL_RGB;
                if (channels == 4)
                    format = GL_RGBA;
                else if (channels == 1)
                    format = GL_RED;
                
                glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, decoded_data);
                glGenerateMipmap(GL_TEXTURE_2D);
                
                stbi_image_free(decoded_data);
                glBindTexture(GL_TEXTURE_2D, 0);
                
                loaded_textures.push_back(texture);
                std::cout << "Loaded embedded texture " << static_cast<unsigned int>(img_idx) << " (" << width << "x" << height << ", " << channels << " channels)\n";
            }
        }
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
            cgltf_accessor* texcoord_accessor = nullptr;

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
                else if (attr->type == cgltf_attribute_type_texcoord)
                {
                    texcoord_accessor = attr->data;
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

            // Unpack texture coordinates if available
            std::vector<float> texcoords;
            if (texcoord_accessor && texcoord_accessor->count == vertex_count)
            {
                // Suppress warning: cgltf_num_components returns small values (1-4), safe to cast
                #pragma warning(push)
                #pragma warning(disable: 4267)
                const size_t num_components = static_cast<size_t>(cgltf_num_components(texcoord_accessor->type));
                #pragma warning(pop)
                texcoords.resize(vertex_count * num_components);
                cgltf_accessor_unpack_floats(texcoord_accessor, texcoords.data(), 
                    static_cast<cgltf_size>(vertex_count * num_components));
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

                // Use color from accessor if available, otherwise use default gray-ish color
                if (!colors.empty())
                {
                    // Suppress warning: cgltf_num_components returns small values (1-4), safe to cast
                    #pragma warning(push)
                    #pragma warning(disable: 4267)
                    const size_t num_components = static_cast<size_t>(cgltf_num_components(color_accessor->type));
                    #pragma warning(pop)
                    const size_t base_idx = v * num_components;
                    vertex.color = glm::vec3(
                        colors[base_idx + 0],
                        num_components > 1 ? colors[base_idx + 1] : colors[base_idx + 0],
                        num_components > 2 ? colors[base_idx + 2] : colors[base_idx + 0]
                    );
                }
                else
                {
                    // Default to light gray if no color data (instead of pure white)
                    // This helps distinguish the model even without textures
                    vertex.color = glm::vec3(0.8f, 0.8f, 0.8f);
                }

                // Set texture coordinates if available
                if (!texcoords.empty())
                {
                    // Suppress warning: cgltf_num_components returns small values (1-4), safe to cast
                    #pragma warning(push)
                    #pragma warning(disable: 4267)
                    const size_t num_components = static_cast<size_t>(cgltf_num_components(texcoord_accessor->type));
                    #pragma warning(pop)
                    const size_t base_idx = v * num_components;
                    vertex.texcoord = glm::vec2(
                        texcoords[base_idx + 0],
                        num_components > 1 ? texcoords[base_idx + 1] : 0.0f
                    );
                }
                else
                {
                    vertex.texcoord = glm::vec2(0.0f, 0.0f);
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
                
                // Try to find texture for this primitive's material
                if (primitive->material)
                {
                    const cgltf_material* mat = primitive->material;
                    if (mat->pbr_metallic_roughness.base_color_texture.texture)
                    {
                        const cgltf_texture* tex = mat->pbr_metallic_roughness.base_color_texture.texture;
                        if (tex->image && tex->image - data->images < static_cast<ptrdiff_t>(loaded_textures.size()))
                        {
                            // Store texture index with mesh (for now, just store all textures)
                            // In a more sophisticated system, you'd store texture index per mesh
                        }
                    }
                }
            }
        }
    }

    // Store all loaded textures in model
    model.textures = std::move(loaded_textures);

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
    
    for (auto& texture : model.textures)
    {
        destroy_texture(texture);
    }
    model.textures.clear();
}

