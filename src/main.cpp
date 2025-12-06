 #include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <filesystem>
#include <iostream>
#include <stdexcept>

#include "core/camera.h"
#include "core/window.h"
#include "game/game_state.h"
#include "game/game_loop.h"
#include "game/renderer.h"
#include "rendering/shader.h"
#include "rendering/text_renderer.h"
#include "rendering/texture_loader.h"
#include "rendering/gltf_loader.h"
#include "rendering/obj_loader.h"
#include "utils/file_utils.h"
#include "game/menu/menu_renderer.h"

namespace
{
    void load_dice_assets(const std::filesystem::path& executable_dir, 
                         const std::filesystem::path& source_dir,
                         game::GameState& game_state)
    {
        // Try GLB format first
        std::filesystem::path dice_glb_path = source_dir / "game" / "player" / "dice" / "source" / "dice.glb";
        std::filesystem::path dice_obj_path = source_dir / "game" / "player" / "dice" / "source" / "dice.7z" / "dice.obj";

        try
        {
            if (std::filesystem::exists(dice_glb_path))
            {
                std::cout << "Loading dice model (GLB) from: " << dice_glb_path << std::endl;
                game_state.dice_model_glb = load_gltf_model(dice_glb_path);
                game_state.has_dice_model = true;
                game_state.is_obj_format = false;
                std::cout << "Loaded dice model with " << game_state.dice_model_glb.meshes.size() << " mesh(es)\n";
            }
            else if (std::filesystem::exists(dice_obj_path))
            {
                std::cout << "Loading dice model (OBJ) from: " << dice_obj_path << std::endl;
                game_state.dice_model_obj = load_obj_model(dice_obj_path);
                game_state.has_dice_model = true;
                game_state.is_obj_format = true;
                std::cout << "Loaded dice model with " << game_state.dice_model_obj.meshes.size() << " mesh(es)\n";
            }
            else if (std::filesystem::exists(executable_dir / "dice.glb"))
            {
                std::cout << "Loading dice model (GLB) from executable directory\n";
                game_state.dice_model_glb = load_gltf_model(executable_dir / "dice.glb");
                game_state.has_dice_model = true;
                game_state.is_obj_format = false;
                std::cout << "Loaded dice model with " << game_state.dice_model_glb.meshes.size() << " mesh(es)\n";
            }
            else
            {
                std::cerr << "Warning: Dice model file not found\n";
            }
        }
        catch (const std::exception& ex)
        {
            std::cerr << "Warning: Failed to load dice model: " << ex.what() << '\n';
        }

        // Load dice texture
        try
        {
            std::filesystem::path texture_png_path = source_dir / "game" / "player" / "dice" / "textures" / "cost.png";
            if (std::filesystem::exists(texture_png_path))
            {
                std::cout << "Loading dice texture from: " << texture_png_path << std::endl;
                game_state.dice_texture = load_texture(texture_png_path);
                game_state.has_dice_texture = true;
                std::cout << "Dice texture loaded successfully! ID: " << game_state.dice_texture.id << std::endl;
            }
        }
        catch (const std::exception& ex)
        {
            std::cerr << "Warning: Failed to load dice texture: " << ex.what() << '\n';
        }
    }
}

int main(int argc, char* argv[])
{
    try
    {
        // Initialize window
        core::Window window(800, 600, "Pacman OpenGL");
        
        // Setup camera
        core::Camera camera;
        window.set_scroll_callback([&camera](double /*x_offset*/, double y_offset) {
            camera.adjust_fov(static_cast<float>(y_offset));
        });

        // Get executable directory
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

        // Load shaders
        const auto shaders_dir = executable_dir / "shaders";
        const std::string vertex_source = load_file(shaders_dir / "simple.vert");
        const std::string fragment_source = load_file(shaders_dir / "simple.frag");
        const GLuint program = create_program(vertex_source, fragment_source);
        const GLint mvp_location = glGetUniformLocation(program, "uMVP");
        const GLint use_texture_location = glGetUniformLocation(program, "uUseTexture");
        const GLint texture_location = glGetUniformLocation(program, "uTexture");
        const GLint dice_texture_mode_location = glGetUniformLocation(program, "uDiceTextureMode");
        
        glUseProgram(program);
        if (texture_location >= 0)
        {
            glUniform1i(texture_location, 0);
        }
        if (dice_texture_mode_location >= 0)
        {
            glUniform1i(dice_texture_mode_location, 0);
        }
        glEnable(GL_DEPTH_TEST);

        // Initialize game state
        game::GameState game_state;
        game::initialize_game_state(game_state, executable_dir);

        // Load dice assets
        std::filesystem::path source_dir = std::filesystem::path(__FILE__).parent_path();
        load_dice_assets(executable_dir, source_dir, game_state);

        // Initialize text renderer
        game::RenderState render_state;
        render_state.program = program;
        render_state.mvp_location = mvp_location;
        render_state.use_texture_location = use_texture_location;
        render_state.texture_location = texture_location;
        render_state.dice_texture_mode_location = dice_texture_mode_location;

        std::filesystem::path font_path = executable_dir / "pixel-game.regular.otf";
        if (!std::filesystem::exists(font_path))
        {
            std::filesystem::path source_font = source_dir.parent_path() / "assets" / "fonts" / "pixel-game.regular.otf";
            if (std::filesystem::exists(source_font))
            {
                font_path = source_font;
            }
        }
        if (!std::filesystem::exists(font_path))
        {
            throw std::runtime_error("Font pixel-game.regular.otf not found.");
        }
        if (!initialize_text_renderer(render_state.text_renderer, font_path.string(), 72))
        {
            throw std::runtime_error("Failed to initialize text renderer.");
        }

        // Load menu textures
        std::filesystem::path assets_dir = source_dir.parent_path() / "assets";
        if (!std::filesystem::exists(assets_dir))
        {
            assets_dir = executable_dir / "assets";
        }
        if (!game::menu::load_menu_textures(assets_dir))
        {
            std::cerr << "Warning: Failed to load menu textures, menu will not be displayed\n";
        }

        // Initialize game loop and renderer
        game::GameLoop game_loop(window, camera, game_state, render_state);
        game::Renderer renderer(render_state);

        // Main game loop
        game_state.last_time = static_cast<float>(glfwGetTime());
        while (!window.should_close())
        {
            const float current_time = static_cast<float>(glfwGetTime());
            const float delta_time = current_time - game_state.last_time;
            game_state.last_time = current_time;

            window.poll_events();

            if (window.is_key_pressed(GLFW_KEY_ESCAPE))
            {
                window.close();
            }

            // Update game
            game_loop.update(delta_time);

            // Render
            renderer.render(window, camera, game_state);

            window.swap_buffers();
        }

        // Cleanup
        game::menu::destroy_menu_textures();
        game::cleanup_game_state(game_state);
        destroy_text_renderer(render_state.text_renderer);
        glDeleteProgram(program);
    }
    catch (const std::exception& ex)
    {
        std::cerr << ex.what() << '\n';
        return 1;
    }
    return 0;
}

