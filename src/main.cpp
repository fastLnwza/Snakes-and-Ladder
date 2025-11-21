#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <filesystem>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "core/camera.h"
#include "core/types.h"
#include "core/window.h"
#include "game/map/board.h"
#include "game/map/map_generator.h"
#include "game/player/player.h"
#include "game/player/dice/dice.h"
#include "rendering/gltf_loader.h"
#include "rendering/obj_loader.h"
#include "rendering/mesh.h"
#include "rendering/primitives.h"
#include "rendering/shader.h"
#include "rendering/texture_loader.h"
#include "utils/file_utils.h"

int main(int argc, char* argv[])
{
    try
    {
        // Initialize window
        core::Window window(800, 600, "Pacman OpenGL");
        
        // Setup camera scroll callback
        core::Camera camera;
        window.set_scroll_callback([&camera](double /*x_offset*/, double y_offset) {
            camera.adjust_fov(static_cast<float>(y_offset));
        });

        // Load shaders
        std::filesystem::path executable_dir = std::filesystem::current_path();
        if (argc > 0)
        {
            std::error_code ec;
            auto resolved = std::filesystem::weakly_canonical(std::filesystem::path(argv[0]), ec);
            if (!ec)
            {
                executable_dir = resolved.parent_path();
            }
        }

        const auto shaders_dir = executable_dir / "shaders";
        const std::string vertex_source = load_file(shaders_dir / "simple.vert");
        const std::string fragment_source = load_file(shaders_dir / "simple.frag");
        const GLuint program = create_program(vertex_source, fragment_source);
        const GLint mvp_location = glGetUniformLocation(program, "uMVP");
        const GLint use_texture_location = glGetUniformLocation(program, "uUseTexture");
        const GLint texture_location = glGetUniformLocation(program, "uTexture");
        
        // Set texture sampler to use texture unit 0
        glUseProgram(program);
        if (texture_location >= 0)
        {
            glUniform1i(texture_location, 0); // Bind to texture unit 0
        }

        glEnable(GL_DEPTH_TEST);

        // Build map
        using namespace game::map;
        const auto [map_vertices, map_indices] = build_snakes_ladders_map();
        Mesh map_mesh = create_mesh(map_vertices, map_indices);

        const float board_width = static_cast<float>(BOARD_COLUMNS) * TILE_SIZE;
        const float board_height = static_cast<float>(BOARD_ROWS) * TILE_SIZE;
        const float map_length = board_height;
        const float map_min_dimension = std::min(board_width, board_height);

        // Create player
        using namespace game::player;
        const float player_radius = std::max(0.4f, 0.025f * map_min_dimension);
        const auto [sphere_vertices, sphere_indices] = build_sphere(player_radius, 32, 16, {1.0f, 0.9f, 0.1f});
        Mesh sphere_mesh = create_mesh(sphere_vertices, sphere_indices);

        const float player_ground_y = player_radius;
        PlayerState player_state;
        initialize(player_state, tile_center_world(0), player_ground_y, player_radius);
        const int final_tile_index = BOARD_COLUMNS * BOARD_ROWS - 1;

        // Load dice model from dice folder (support both GLB and OBJ)
        GLTFModel dice_model_glb;
        OBJModel dice_model_obj;
        bool has_dice_model = false;
        bool is_obj_format = false;
        
        try
        {
            // Calculate path to dice source folder relative to source directory
            // From src/main.cpp -> src/ -> src/game/player/dice/source/
            std::filesystem::path source_dir = std::filesystem::path(__FILE__).parent_path();
            
            // Try GLB format first
            std::filesystem::path dice_glb_path = source_dir / "game" / "player" / "dice" / "source" / "dice.glb";
            std::filesystem::path dice_obj_path = source_dir / "game" / "player" / "dice" / "source" / "dice.7z" / "dice.obj";
            
            if (std::filesystem::exists(dice_glb_path))
            {
                std::cout << "Loading dice model (GLB) from: " << dice_glb_path << std::endl;
                dice_model_glb = load_gltf_model(dice_glb_path);
                has_dice_model = true;
                is_obj_format = false;
                std::cout << "Loaded dice model with " << dice_model_glb.meshes.size() << " mesh(es)\n";
            }
            else if (std::filesystem::exists(dice_obj_path))
            {
                std::cout << "Loading dice model (OBJ) from: " << dice_obj_path << std::endl;
                dice_model_obj = load_obj_model(dice_obj_path);
                has_dice_model = true;
                is_obj_format = true;
                std::cout << "Loaded dice model with " << dice_model_obj.meshes.size() << " mesh(es)\n";
            }
            // Fallback: try executable directory
            else if (std::filesystem::exists(executable_dir / "dice.glb"))
            {
                std::cout << "Loading dice model (GLB) from executable directory\n";
                dice_model_glb = load_gltf_model(executable_dir / "dice.glb");
                has_dice_model = true;
                is_obj_format = false;
                std::cout << "Loaded dice model with " << dice_model_glb.meshes.size() << " mesh(es)\n";
            }
            else
            {
                std::cerr << "Warning: Dice model file not found\n";
                std::cerr << "Tried:\n";
                std::cerr << "  - " << dice_glb_path << "\n";
                std::cerr << "  - " << dice_obj_path << "\n";
                std::cerr << "  - " << (executable_dir / "dice.glb") << "\n";
                std::cerr << "Dice visualization will be disabled.\n";
            }
        }
        catch (const std::exception& ex)
        {
            std::cerr << "Warning: Failed to load dice model: " << ex.what() << '\n';
            std::cerr << "Dice visualization will be disabled.\n";
        }

        // Load dice texture (try multiple locations)
        Texture dice_texture{};
        bool has_dice_texture = false;
        try
        {
            std::filesystem::path source_dir = std::filesystem::path(__FILE__).parent_path();
            std::filesystem::path texture_png_path = source_dir / "game" / "player" / "dice" / "textures" / "cost.png";
            std::filesystem::path texture_dds_path = source_dir / "game" / "player" / "dice" / "source" / "dice.7z" / "cost.dds";
            
            if (std::filesystem::exists(texture_png_path))
            {
                std::cout << "Loading dice texture from: " << texture_png_path << std::endl;
                dice_texture = load_texture(texture_png_path);
                has_dice_texture = true;
                std::cout << "Dice texture loaded successfully! ID: " << dice_texture.id << std::endl;
            }
            else if (std::filesystem::exists(texture_dds_path))
            {
                std::cout << "DDS texture format detected but not supported. Please convert to PNG.\n";
                std::cout << "Found DDS at: " << texture_dds_path << std::endl;
            }
            else
            {
                std::cerr << "Warning: Dice texture not found at:\n";
                std::cerr << "  - " << texture_png_path << "\n";
                std::cerr << "  - " << texture_dds_path << "\n";
            }
        }
        catch (const std::exception& ex)
        {
            std::cerr << "Warning: Failed to load dice texture: " << ex.what() << '\n';
        }

        // Initialize dice state
        using namespace game::player::dice;
        DiceState dice_state;
        const glm::vec3 dice_position = tile_center_world(0) + glm::vec3(0.0f, player_ground_y + 3.0f, 0.0f);
        initialize(dice_state, dice_position);
        dice_state.scale = player_radius * 0.167f; // Reduced scale
        dice_state.rotation = glm::vec3(45.0f, 45.0f, 0.0f); // Initial rotation to make it visible

        float last_time = 0.0f;

        // Main game loop
        last_time = static_cast<float>(glfwGetTime());
        while (!window.should_close())
        {
            const float current_time = static_cast<float>(glfwGetTime());
            const float delta_time = current_time - last_time;
            last_time = current_time;

            window.poll_events();

            if (window.is_key_pressed(GLFW_KEY_ESCAPE))
            {
                window.close();
            }

            // Update player
            const bool space_pressed = window.is_key_pressed(GLFW_KEY_SPACE);
            const bool space_just_pressed = space_pressed && !player_state.previous_space_state;
            
            // Check if dice should start rolling (before player update to catch the roll)
            int previous_dice_result = player_state.last_dice_result;
            
            update(player_state, delta_time, space_just_pressed, final_tile_index);
            player_state.previous_space_state = space_pressed;
            
            // Start dice roll animation when a new dice result is generated
            if (player_state.last_dice_result != previous_dice_result && player_state.last_dice_result > 0)
            {
                start_roll(dice_state, player_state.last_dice_result);
            }
            
            // Update dice animation
            update(dice_state, delta_time);

            // Render
            glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            const float aspect_ratio = window.get_aspect_ratio();
            const glm::mat4 projection = camera.get_projection(aspect_ratio);
            const glm::vec3 player_position = get_position(player_state);
            const glm::mat4 view = camera.get_view(player_position, map_length);

            glUseProgram(program);
            
            // Disable texture for map and player (they use vertex colors only)
            glUniform1i(use_texture_location, 0);
            glBindTexture(GL_TEXTURE_2D, 0);

            // Draw map
            {
                const glm::mat4 model(1.0f);
                const glm::mat4 mvp = projection * view * model;
                glUniformMatrix4fv(mvp_location, 1, GL_FALSE, glm::value_ptr(mvp));
                glBindVertexArray(map_mesh.vao);
                glDrawElements(GL_TRIANGLES, map_mesh.index_count, GL_UNSIGNED_INT, nullptr);
            }

            // Draw player
            {
                const glm::mat4 model = glm::translate(glm::mat4(1.0f), player_position);
                const glm::mat4 mvp = projection * view * model;
                glUniformMatrix4fv(mvp_location, 1, GL_FALSE, glm::value_ptr(mvp));
                glBindVertexArray(sphere_mesh.vao);
                glDrawElements(GL_TRIANGLES, sphere_mesh.index_count, GL_UNSIGNED_INT, nullptr);
            }

            // Draw dice (always visible for now to test)
            const std::vector<Mesh>* dice_meshes = nullptr;
            if (has_dice_model)
            {
                if (is_obj_format)
                {
                    if (!dice_model_obj.meshes.empty())
                        dice_meshes = &dice_model_obj.meshes;
                }
                else
                {
                    if (!dice_model_glb.meshes.empty())
                        dice_meshes = &dice_model_glb.meshes;
                }
            }
            
            if (dice_meshes && !dice_meshes->empty())
            {
                // Position dice at player's current tile center to ensure it's on the board
                const int current_tile = get_current_tile(player_state);
                // Clamp current_tile to valid range
                const int clamped_tile = std::clamp(current_tile, 0, BOARD_COLUMNS * BOARD_ROWS - 1);
                const glm::vec3 tile_center = tile_center_world(clamped_tile, player_ground_y);
                
                // Position dice above the tile center - ensure it stays on board
                glm::vec3 dice_pos = tile_center;
                dice_pos.y += player_radius * 1.5f; // Position above tile
                
                // Calculate board bounds using same logic as tile_center_world
                const float half_width = board_width * 0.5f;
                const float half_height = board_height * 0.5f;
                const float dice_radius = dice_state.scale * 0.5f;
                
                // Clamp dice position to stay strictly within board bounds
                dice_pos.x = std::clamp(dice_pos.x, -half_width + dice_radius, half_width - dice_radius);
                dice_pos.z = std::clamp(dice_pos.z, -half_height + dice_radius, half_height - dice_radius);
                
                dice_state.position = dice_pos;
                
                const glm::mat4 dice_transform = get_transform(dice_state);
                const glm::mat4 dice_mvp = projection * view * dice_transform;
                glUniformMatrix4fv(mvp_location, 1, GL_FALSE, glm::value_ptr(dice_mvp));
                
                // Enable texture if available
                if (has_dice_texture && dice_texture.id != 0)
                {
                    glUseProgram(program); // Make sure shader is active
                    glUniform1i(use_texture_location, 1); // Enable texture
                    glUniform1i(texture_location, 0); // Set sampler to texture unit 0
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, dice_texture.id);
                }
                else
                {
                    glUniform1i(use_texture_location, 0); // Disable texture
                }
                
                // Draw only once - use first mesh only to avoid duplicates
                if (!dice_meshes->empty())
                {
                    // Only draw the first mesh to avoid rendering duplicates
                    glBindVertexArray((*dice_meshes)[0].vao);
                    glDrawElements(GL_TRIANGLES, (*dice_meshes)[0].index_count, GL_UNSIGNED_INT, nullptr);
                }
                
                // Unbind texture
                if (has_dice_texture)
                {
                    glBindTexture(GL_TEXTURE_2D, 0);
                }
            }

            window.swap_buffers();
        }

        // Cleanup
        if (has_dice_texture)
        {
            destroy_texture(dice_texture);
        }
        destroy_mesh(map_mesh);
        destroy_mesh(sphere_mesh);
        if (has_dice_model)
        {
            if (is_obj_format)
            {
                if (!dice_model_obj.meshes.empty())
                {
                    destroy_obj_model(dice_model_obj);
                }
            }
            else
            {
                if (!dice_model_glb.meshes.empty())
                {
                    destroy_gltf_model(dice_model_glb);
                }
            }
        }
        glDeleteProgram(program);
    }
    catch (const std::exception& ex)
    {
        std::cerr << ex.what() << '\n';
        return 1;
    }
    return 0;
}
