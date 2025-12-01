#include "obj_loader.h"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "rendering/geometry/mesh.h"
#include "utils/file_utils.h"

OBJModel load_obj_model(const std::filesystem::path& path)
{
    OBJModel model;

    std::ifstream file(path);
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open OBJ file: " + path.string());
    }

    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texcoords;
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#')
        {
            continue;
        }

        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;

        if (prefix == "v") // Vertex position
        {
            glm::vec3 pos;
            iss >> pos.x >> pos.y >> pos.z;
            positions.push_back(pos);
        }
        else if (prefix == "vn") // Vertex normal
        {
            glm::vec3 norm;
            iss >> norm.x >> norm.y >> norm.z;
            normals.push_back(norm);
        }
        else if (prefix == "vt") // Texture coordinate
        {
            glm::vec2 tex;
            iss >> tex.x >> tex.y;
            texcoords.push_back(tex);
        }
        else if (prefix == "f") // Face
        {
            std::string vertex_str;
            std::vector<unsigned int> face_indices;

            while (iss >> vertex_str)
            {
                std::istringstream vss(vertex_str);
                std::string token;
                
                int pos_idx = -1, tex_idx = -1, norm_idx = -1;
                int component = 0;

                while (std::getline(vss, token, '/'))
                {
                    if (!token.empty())
                    {
                        int idx = std::stoi(token) - 1; // OBJ uses 1-based indexing
                        
                        if (component == 0)
                            pos_idx = idx;
                        else if (component == 1)
                            tex_idx = idx;
                        else if (component == 2)
                            norm_idx = idx;
                    }
                    component++;
                }

                if (pos_idx >= 0 && pos_idx < static_cast<int>(positions.size()))
                {
                    Vertex vertex{};
                    vertex.position = positions[pos_idx];

                    // Use texture coordinate if available
                    if (tex_idx >= 0 && tex_idx < static_cast<int>(texcoords.size()))
                    {
                        vertex.texcoord = texcoords[tex_idx];
                        // Flip V coordinate (OBJ uses bottom-left origin, OpenGL uses top-left)
                        vertex.texcoord.y = 1.0f - vertex.texcoord.y;
                    }

                    // Use normal if available
                    if (norm_idx >= 0 && norm_idx < static_cast<int>(normals.size()))
                    {
                        // Normals can be used for lighting, but we just use default color for now
                    }

                    // Default white color
                    vertex.color = glm::vec3(1.0f, 1.0f, 1.0f);

                    // Try to find existing vertex (must match position AND texture coordinate)
                    bool found = false;
                    unsigned int existing_idx = 0;
                    for (size_t i = 0; i < vertices.size(); ++i)
                    {
                        if (vertices[i].position == vertex.position && 
                            vertices[i].texcoord == vertex.texcoord)
                        {
                            found = true;
                            existing_idx = static_cast<unsigned int>(i);
                            break;
                        }
                    }

                    if (found)
                    {
                        face_indices.push_back(existing_idx);
                    }
                    else
                    {
                        unsigned int new_idx = static_cast<unsigned int>(vertices.size());
                        vertices.push_back(vertex);
                        face_indices.push_back(new_idx);
                    }
                }
            }

            // Triangulate face (assuming simple triangular faces)
            if (face_indices.size() >= 3)
            {
                for (size_t i = 1; i < face_indices.size() - 1; ++i)
                {
                    indices.push_back(face_indices[0]);
                    indices.push_back(face_indices[i]);
                    indices.push_back(face_indices[i + 1]);
                }
            }
        }
    }

    file.close();

    // Create mesh from vertices and indices
    if (!vertices.empty() && !indices.empty())
    {
        Mesh mesh = create_mesh(vertices, indices);
        model.meshes.push_back(mesh);
    }

    return model;
}

void destroy_obj_model(OBJModel& model)
{
    for (auto& mesh : model.meshes)
    {
        destroy_mesh(mesh);
    }
    model.meshes.clear();
}

