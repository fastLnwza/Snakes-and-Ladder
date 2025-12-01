#include "game_initializer.h"

#include "core/window.h"
#include "core/camera.h"
#include "game/map/board.h"
#include "game/map/map_generator.h"
#include "game/player/player.h"
#include "game/player/dice/dice.h"
#include "rendering/geometry/primitives.h"
#include "rendering/geometry/mesh.h"
#include "rendering/shader/shader.h"
#include "rendering/loaders/gltf_loader.h"
#include "rendering/loaders/obj_loader.h"
#include "rendering/loaders/texture_loader.h"
#include "rendering/text/text_renderer.h"
#include "utils/file_utils.h"

#include <glad/glad.h>
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace game
{
    namespace
    {
        std::filesystem::path get_executable_dir(int argc, char* argv[])
        {
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
            return executable_dir;
        }
    }

    InitializationResult initialize_game(GameState& state, int argc, char* argv[])
    {
        try
        {
            // Initialize window
            static core::Window window(800, 600, "Pacman OpenGL");
            state.window = &window;
            
            // Setup camera scroll callback
            window.set_scroll_callback([&state](double /*x_offset*/, double y_offset) {
                state.camera.adjust_fov(static_cast<float>(y_offset));
            });

            // Load shaders
            const auto executable_dir = get_executable_dir(argc, argv);
            const auto shaders_dir = executable_dir / "shaders";
            const std::string vertex_source = load_file(shaders_dir / "simple.vert");
            const std::string fragment_source = load_file(shaders_dir / "simple.frag");
            state.shader_program = create_program(vertex_source, fragment_source);
            state.mvp_location = glGetUniformLocation(state.shader_program, "uMVP");
            state.use_texture_location = glGetUniformLocation(state.shader_program, "uUseTexture");
            state.texture_location = glGetUniformLocation(state.shader_program, "uTexture");
            state.dice_texture_mode_location = glGetUniformLocation(state.shader_program, "uDiceTextureMode");
            
            // Set texture sampler to use texture unit 0
            glUseProgram(state.shader_program);
            if (state.texture_location >= 0)
            {
                glUniform1i(state.texture_location, 0);
            }
            if (state.dice_texture_mode_location >= 0)
            {
                glUniform1i(state.dice_texture_mode_location, 0);
            }

            glEnable(GL_DEPTH_TEST);

            // Build map
            using namespace game::map;
            const auto [map_vertices, map_indices] = build_snakes_ladders_map();
            state.map_mesh = create_mesh(map_vertices, map_indices);

            state.board_width = static_cast<float>(BOARD_COLUMNS) * TILE_SIZE;
            state.board_height = static_cast<float>(BOARD_ROWS) * TILE_SIZE;
            state.map_length = state.board_height;
            const float map_min_dimension = std::min(state.board_width, state.board_height);
            state.final_tile_index = BOARD_COLUMNS * BOARD_ROWS - 1;

            // Create player
            using namespace game::player;
            state.player_radius = std::max(0.4f, 0.025f * map_min_dimension);
            const auto [sphere_vertices, sphere_indices] = build_sphere(state.player_radius, 32, 16, {1.0f, 0.9f, 0.1f});
            state.player_mesh = create_mesh(sphere_vertices, sphere_indices);

            state.player_ground_y = state.player_radius;
            initialize(state.player_state, tile_center_world(0), state.player_ground_y, state.player_radius);

            // Load dice model
            try
            {
                std::filesystem::path source_dir = std::filesystem::path(__FILE__).parent_path().parent_path();
                std::filesystem::path dice_glb_path = source_dir / "game" / "player" / "dice" / "source" / "dice.glb";
                std::filesystem::path dice_obj_path = source_dir / "game" / "player" / "dice" / "source" / "dice.7z" / "dice.obj";
                
                if (std::filesystem::exists(dice_glb_path))
                {
                    std::cout << "Loading dice model (GLB) from: " << dice_glb_path << std::endl;
                    state.dice_model_glb = load_gltf_model(dice_glb_path);
                    state.has_dice_model = true;
                    state.is_obj_format = false;
                    std::cout << "Loaded dice model with " << state.dice_model_glb.meshes.size() << " mesh(es)\n";
                }
                else if (std::filesystem::exists(dice_obj_path))
                {
                    std::cout << "Loading dice model (OBJ) from: " << dice_obj_path << std::endl;
                    state.dice_model_obj = load_obj_model(dice_obj_path);
                    state.has_dice_model = true;
                    state.is_obj_format = true;
                    std::cout << "Loaded dice model with " << state.dice_model_obj.meshes.size() << " mesh(es)\n";
                }
                else if (std::filesystem::exists(executable_dir / "dice.glb"))
                {
                    std::cout << "Loading dice model (GLB) from executable directory\n";
                    state.dice_model_glb = load_gltf_model(executable_dir / "dice.glb");
                    state.has_dice_model = true;
                    state.is_obj_format = false;
                    std::cout << "Loaded dice model with " << state.dice_model_glb.meshes.size() << " mesh(es)\n";
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

            // Load dice texture
            try
            {
                std::filesystem::path source_dir = std::filesystem::path(__FILE__).parent_path().parent_path();
                std::filesystem::path texture_png_path = source_dir / "game" / "player" / "dice" / "textures" / "cost.png";
                std::filesystem::path texture_dds_path = source_dir / "game" / "player" / "dice" / "source" / "dice.7z" / "cost.dds";
                
                if (std::filesystem::exists(texture_png_path))
                {
                    std::cout << "Loading dice texture from: " << texture_png_path << std::endl;
                    state.dice_texture = load_texture(texture_png_path);
                    state.has_dice_texture = true;
                    std::cout << "Dice texture loaded successfully! ID: " << state.dice_texture.id << std::endl;
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
            const glm::vec3 dice_position = tile_center_world(0) + glm::vec3(0.0f, state.player_ground_y + 3.0f, 0.0f);
            initialize(state.dice_state, dice_position);
            state.dice_state.scale = state.player_radius * 0.167f;
            state.dice_state.rotation = glm::vec3(45.0f, 45.0f, 0.0f);
            state.dice_state.ground_y = state.player_ground_y;

            // Initialize text renderer
            std::filesystem::path font_path = executable_dir / "pixel-game.regular.otf";
            if (!std::filesystem::exists(font_path))
            {
                // Try source directory (project root)
                std::filesystem::path source_dir = std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();
                std::filesystem::path source_font = source_dir / "pixel-game.regular.otf";
                if (std::filesystem::exists(source_font))
                {
                    font_path = source_font;
                }
            }
            if (!std::filesystem::exists(font_path))
            {
                return {false, "Font pixel-game.regular.otf not found."};
            }
            if (!initialize_text_renderer(state.text_renderer, font_path.string(), 72))
            {
                return {false, "Failed to initialize text renderer."};
            }

            // Initialize game state
            state.last_processed_tile = get_current_tile(state.player_state);
            state.precision_result_display_timer = 3.0f;
            state.tile_memory_previous_keys.fill(false);

            return {true, ""};
        }
        catch (const std::exception& ex)
        {
            return {false, ex.what()};
        }
    }

    void cleanup_game(GameState& state)
    {
        destroy_text_renderer(state.text_renderer);
        if (state.has_dice_texture)
        {
            destroy_texture(state.dice_texture);
        }
        destroy_mesh(state.map_mesh);
        destroy_mesh(state.player_mesh);
        if (state.has_dice_model)
        {
            if (state.is_obj_format)
            {
                if (!state.dice_model_obj.meshes.empty())
                {
                    destroy_obj_model(state.dice_model_obj);
                }
            }
            else
            {
                if (!state.dice_model_glb.meshes.empty())
                {
                    destroy_gltf_model(state.dice_model_glb);
                }
            }
        }
        if (state.shader_program != 0)
        {
            glDeleteProgram(state.shader_program);
        }
    }
}

