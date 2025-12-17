#include "fbx_loader.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <stdexcept>
#include <iostream>
#include <algorithm>

#include "mesh.h"
#include "texture_loader.h"
#include "utils/file_utils.h"
#include "../../external/stb_image.h"

// Helper function to convert Assimp matrix to GLM matrix
glm::mat4 aiMatrix4x4ToGlm(const aiMatrix4x4& ai_mat)
{
    glm::mat4 result;
    result[0][0] = ai_mat.a1; result[0][1] = ai_mat.b1; result[0][2] = ai_mat.c1; result[0][3] = ai_mat.d1;
    result[1][0] = ai_mat.a2; result[1][1] = ai_mat.b2; result[1][2] = ai_mat.c2; result[1][3] = ai_mat.d2;
    result[2][0] = ai_mat.a3; result[2][1] = ai_mat.b3; result[2][2] = ai_mat.c3; result[2][3] = ai_mat.d3;
    result[3][0] = ai_mat.a4; result[3][1] = ai_mat.b4; result[3][2] = ai_mat.c4; result[3][3] = ai_mat.d4;
    return result;
}

// Helper function to convert Assimp vector to GLM vector
glm::vec3 aiVector3DToGlm(const aiVector3D& vec)
{
    return glm::vec3(vec.x, vec.y, vec.z);
}

// Load a single mesh from Assimp
Mesh load_mesh_from_ai(const aiMesh* ai_mesh)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    // Extract vertices
    for (unsigned int i = 0; i < ai_mesh->mNumVertices; ++i)
    {
        Vertex vertex;
        vertex.position = aiVector3DToGlm(ai_mesh->mVertices[i]);
        
        // Try to get vertex colors from mesh if available
        if (ai_mesh->HasVertexColors(0))
        {
            const aiColor4D* colors = ai_mesh->mColors[0];
            vertex.color = glm::vec3(colors[i].r, colors[i].g, colors[i].b);
        }
        else
        {
            // Default to a visible color (not white) if no texture
            vertex.color = glm::vec3(0.6f, 0.4f, 0.2f);  // Brown color as fallback
        }
        
        if (ai_mesh->mTextureCoords[0])
        {
            vertex.texcoord = glm::vec2(ai_mesh->mTextureCoords[0][i].x, ai_mesh->mTextureCoords[0][i].y);
        }
        else
        {
            vertex.texcoord = glm::vec2(0.0f, 0.0f);
        }

        vertices.push_back(vertex);
    }

    // Extract indices
    for (unsigned int i = 0; i < ai_mesh->mNumFaces; ++i)
    {
        const aiFace& face = ai_mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; ++j)
        {
            indices.push_back(face.mIndices[j]);
        }
    }

    return create_mesh(vertices, indices);
}

// Recursively process scene nodes
void process_node(const aiNode* node, const aiScene* scene, FBXModel& model, 
                 const aiMatrix4x4& parent_transform, const std::filesystem::path& model_dir)
{
    aiMatrix4x4 node_transform = parent_transform * node->mTransformation;

    // Process all meshes in this node
    for (unsigned int i = 0; i < node->mNumMeshes; ++i)
    {
        const aiMesh* ai_mesh = scene->mMeshes[node->mMeshes[i]];
        Mesh mesh = load_mesh_from_ai(ai_mesh);
        model.meshes.push_back(mesh);
    }

    // Process children
    for (unsigned int i = 0; i < node->mNumChildren; ++i)
    {
        process_node(node->mChildren[i], scene, model, node_transform, model_dir);
    }
}

// Helper function to find texture file in multiple locations
std::filesystem::path find_texture_file(const std::filesystem::path& model_dir, const std::string& tex_path_str)
{
    std::filesystem::path tex_path(tex_path_str);
    
    // List of locations to search
    std::vector<std::filesystem::path> search_paths;
    
    // 1. Original path (if absolute)
    if (tex_path.is_absolute())
    {
        search_paths.push_back(tex_path);
    }
    
    // 2. Relative to model directory (original relative path)
    search_paths.push_back(model_dir / tex_path);
    
    // 3. Just filename in model directory
    search_paths.push_back(model_dir / tex_path.filename());
    
    // 4. Parent directory (in case textures are in a parent folder)
    search_paths.push_back(model_dir.parent_path() / tex_path.filename());
    
    // 5. Common texture folder names
    std::vector<std::string> texture_folders = {"textures", "texture", "tex", "images", "image", "img"};
    for (const auto& folder : texture_folders)
    {
        search_paths.push_back(model_dir / folder / tex_path.filename());
        search_paths.push_back(model_dir.parent_path() / folder / tex_path.filename());
    }
    
    // Try each path
    for (const auto& search_path : search_paths)
    {
        if (std::filesystem::exists(search_path))
        {
            return search_path;
        }
    }
    
    // Not found - return empty path
    return std::filesystem::path();
}

// Load embedded texture from Assimp scene
Texture load_embedded_texture(const aiTexture* ai_tex)
{
    Texture texture{};
    
    // Check if texture data is compressed (format like "jpg", "png", etc.)
    if (ai_tex->mHeight == 0)
    {
        // Compressed texture data (stored in mWidth bytes)
        int width, height, channels;
        unsigned char* image_data = stbi_load_from_memory(
            reinterpret_cast<const unsigned char*>(ai_tex->pcData),
            static_cast<int>(ai_tex->mWidth),
            &width, &height, &channels,
            0
        );
        
        if (image_data)
        {
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
            
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, image_data);
            glGenerateMipmap(GL_TEXTURE_2D);
            
            stbi_image_free(image_data);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        else
        {
            throw std::runtime_error("Failed to decode embedded texture data");
        }
    }
    else
    {
        // Uncompressed texture data (RGBA format)
        texture.width = ai_tex->mWidth;
        texture.height = ai_tex->mHeight;
        
        glGenTextures(1, &texture.id);
        glBindTexture(GL_TEXTURE_2D, texture.id);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture.width, texture.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, ai_tex->pcData);
        glGenerateMipmap(GL_TEXTURE_2D);
        
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    
    return texture;
}

// Load textures from materials
void load_textures_from_materials(const aiScene* scene, FBXModel& model, const std::filesystem::path& model_dir)
{
    std::cout << "FBX scene has " << scene->mNumTextures << " embedded texture(s)\n";
    
    // First, try to load all embedded textures directly if materials reference external files
    // Sometimes Assimp doesn't use *index format for embedded textures
    if (scene->mNumTextures > 0 && model.textures.empty())
    {
        std::cout << "Attempting to load embedded textures directly...\n";
        for (unsigned int tex_idx = 0; tex_idx < scene->mNumTextures; ++tex_idx)
        {
            const aiTexture* embedded_tex = scene->mTextures[tex_idx];
            if (embedded_tex)
            {
                try
                {
                    Texture texture = load_embedded_texture(embedded_tex);
                    model.textures.push_back(texture);
                    std::cout << "Loaded embedded FBX texture " << tex_idx << " directly (" << texture.width << "x" << texture.height << ")\n";
                }
                catch (const std::exception& ex)
                {
                    std::cerr << "Warning: Failed to load embedded texture " << tex_idx << " directly: " << ex.what() << std::endl;
                }
            }
        }
    }
    
    for (unsigned int mat_idx = 0; mat_idx < scene->mNumMaterials; ++mat_idx)
    {
        const aiMaterial* material = scene->mMaterials[mat_idx];
        
        // Try to load diffuse texture (most common)
        if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0)
        {
            aiString texture_path;
            aiTextureMapping mapping = aiTextureMapping_UV;
            unsigned int uv_index = 0;
            float blend = 1.0f;
            aiTextureOp op = aiTextureOp_Multiply;
            aiTextureMapMode map_mode[2] = { aiTextureMapMode_Wrap, aiTextureMapMode_Wrap };
            
            if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texture_path, &mapping, &uv_index, &blend, &op, map_mode) == AI_SUCCESS)
            {
                std::string tex_path_str = texture_path.C_Str();
                std::cout << "Material " << mat_idx << " texture path: \"" << tex_path_str << "\"\n";
                
                // Check if this is an embedded texture (path starts with '*')
                if (!tex_path_str.empty() && tex_path_str[0] == '*')
                {
                    // Embedded texture - extract index
                    try
                    {
                        unsigned int tex_index = std::stoul(tex_path_str.substr(1));
                        if (tex_index < scene->mNumTextures)
                        {
                            const aiTexture* embedded_tex = scene->mTextures[tex_index];
                            try
                            {
                                Texture texture = load_embedded_texture(embedded_tex);
                                model.textures.push_back(texture);
                                std::cout << "Loaded embedded FBX texture " << tex_index << " (" << texture.width << "x" << texture.height << ")\n";
                            }
                            catch (const std::exception& ex)
                            {
                                std::cerr << "Warning: Failed to load embedded texture " << tex_index << ": " << ex.what() << std::endl;
                            }
                        }
                        else
                        {
                            std::cerr << "Warning: Embedded texture index " << tex_index << " out of range (max: " << scene->mNumTextures << ")\n";
                        }
                    }
                    catch (const std::exception&)
                    {
                        std::cerr << "Warning: Invalid embedded texture index: " << tex_path_str << std::endl;
                    }
                }
                else
                {
                    // External texture file - try to find it
                    std::filesystem::path full_texture_path = find_texture_file(model_dir, tex_path_str);
                    
                    // Try to load texture
                    if (!full_texture_path.empty() && std::filesystem::exists(full_texture_path))
                    {
                        try
                        {
                            Texture texture = load_texture(full_texture_path);
                            model.textures.push_back(texture);
                            std::cout << "Loaded FBX texture from: " << full_texture_path << " (ID: " << texture.id << ")\n";
                        }
                        catch (const std::exception& ex)
                        {
                            std::cerr << "Warning: Failed to load FBX texture from " << full_texture_path << ": " << ex.what() << std::endl;
                        }
                    }
                    else
                    {
                        // Only show warning if it's a meaningful filename (not just a path fragment)
                        std::filesystem::path filename = std::filesystem::path(tex_path_str).filename();
                        if (!filename.empty() && filename.string() != "." && filename.string() != "..")
                        {
                            std::cerr << "Warning: FBX texture file not found: " << filename.string() 
                                      << " (searched in model directory and common texture folders)\n";
                        }
                    }
                }
            }
        }
        
        // Also try other texture types (but skip if we already loaded textures)
        // Note: We may have loaded embedded textures directly above, so check if we need more
        if (model.textures.empty() || mat_idx < model.textures.size())
        {
            for (int tex_type = aiTextureType_DIFFUSE + 1; tex_type < AI_TEXTURE_TYPE_MAX; ++tex_type)
            {
                if (material->GetTextureCount(static_cast<aiTextureType>(tex_type)) > 0)
                {
                    aiString texture_path;
                    aiTextureMapping mapping = aiTextureMapping_UV;
                    unsigned int uv_index = 0;
                    float blend = 1.0f;
                    aiTextureOp op = aiTextureOp_Multiply;
                    aiTextureMapMode map_mode[2] = { aiTextureMapMode_Wrap, aiTextureMapMode_Wrap };
                    
                    if (material->GetTexture(static_cast<aiTextureType>(tex_type), 0, &texture_path, &mapping, &uv_index, &blend, &op, map_mode) == AI_SUCCESS)
                    {
                        std::string tex_path_str = texture_path.C_Str();
                        
                        // Check if this is an embedded texture
                        if (!tex_path_str.empty() && tex_path_str[0] == '*')
                        {
                            try
                            {
                                unsigned int tex_index = std::stoul(tex_path_str.substr(1));
                                if (tex_index < scene->mNumTextures)
                                {
                                    const aiTexture* embedded_tex = scene->mTextures[tex_index];
                                    try
                                    {
                                        Texture texture = load_embedded_texture(embedded_tex);
                                        model.textures.push_back(texture);
                                        std::cout << "Loaded embedded FBX texture (type " << tex_type << ") " << tex_index << "\n";
                                    }
                                    catch (const std::exception&)
                                    {
                                        // Skip failed textures
                                    }
                                }
                            }
                            catch (const std::exception&)
                            {
                                // Skip invalid indices
                            }
                        }
                        else
                        {
                            // External texture file
                            std::filesystem::path full_texture_path = find_texture_file(model_dir, tex_path_str);
                            
                            if (!full_texture_path.empty() && std::filesystem::exists(full_texture_path))
                            {
                                try
                                {
                                    Texture texture = load_texture(full_texture_path);
                                    model.textures.push_back(texture);
                                    std::cout << "Loaded FBX texture (type " << tex_type << ") from: " << full_texture_path << "\n";
                                }
                                catch (const std::exception&)
                                {
                                    // Skip failed textures
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

FBXModel load_fbx_model(const std::filesystem::path& path)
{
    FBXModel model;

    Assimp::Importer importer;
    
    // Import flags for FBX
    unsigned int import_flags = 
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_FlipUVs |
        aiProcess_CalcTangentSpace |
        aiProcess_LimitBoneWeights;

    const aiScene* scene = importer.ReadFile(path.string(), import_flags);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        throw std::runtime_error("Failed to load FBX file: " + path.string() + " - " + importer.GetErrorString());
    }

    std::cout << "Loading FBX model from: " << path << std::endl;
    std::cout << "Meshes: " << scene->mNumMeshes << std::endl;
    std::cout << "Materials: " << scene->mNumMaterials << std::endl;
    std::cout << "Embedded textures in file: " << scene->mNumTextures << std::endl;
    
    // Debug: Check if model has textures
    for (unsigned int mat_idx = 0; mat_idx < scene->mNumMaterials; ++mat_idx)
    {
        const aiMaterial* material = scene->mMaterials[mat_idx];
        std::cout << "Material " << mat_idx << " has " << material->GetTextureCount(aiTextureType_DIFFUSE) << " diffuse texture(s)\n";
    }

    std::filesystem::path model_dir = path.parent_path();

    // Process scene nodes
    aiMatrix4x4 identity;
    process_node(scene->mRootNode, scene, model, identity, model_dir);

    // Load textures from materials
    load_textures_from_materials(scene, model, model_dir);

    std::cout << "Loaded FBX model with " << model.meshes.size() << " mesh(es) and " 
              << model.textures.size() << " texture(s)\n";

    return model;
}

void destroy_fbx_model(FBXModel& model)
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

