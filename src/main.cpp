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

    void load_player_assets(const std::filesystem::path& executable_dir, 
                           const std::filesystem::path& source_dir,
                           game::GameState& game_state)
    {
        // Try to load player1 GLB model
        std::filesystem::path player1_glb_path = source_dir.parent_path() / "assets" / "character" / "player1" / "peasant_character.glb";

        try
        {
            if (std::filesystem::exists(player1_glb_path))
            {
                std::cout << "Loading player1 model (GLB) from: " << player1_glb_path << std::endl;
                game_state.player_model_glb = load_gltf_model(player1_glb_path);
                game_state.has_player_model = true;
                std::cout << "Loaded player1 model with " << game_state.player_model_glb.meshes.size() << " mesh(es)\n";
            }
            else if (std::filesystem::exists(executable_dir / "assets" / "character" / "player1" / "peasant_character.glb"))
            {
                std::cout << "Loading player1 model (GLB) from executable directory\n";
                game_state.player_model_glb = load_gltf_model(executable_dir / "assets" / "character" / "player1" / "peasant_character.glb");
                game_state.has_player_model = true;
                std::cout << "Loaded player1 model with " << game_state.player_model_glb.meshes.size() << " mesh(es)\n";
            }
            else
            {
                std::cerr << "Warning: Player1 model file not found, using sphere fallback\n";
            }
        }
        catch (const std::exception& ex)
        {
            std::cerr << "Warning: Failed to load player1 model: " << ex.what() << '\n';
        }
    }

    void load_player2_assets(const std::filesystem::path& executable_dir, 
                           const std::filesystem::path& source_dir,
                           game::GameState& game_state)
    {
        // Try to load player2 GLB model
        std::filesystem::path player2_glb_path = source_dir.parent_path() / "assets" / "character" / "player2" / "damsel_character.glb";

        try
        {
            if (std::filesystem::exists(player2_glb_path))
            {
                std::cout << "Loading player2 model (GLB) from: " << player2_glb_path << std::endl;
                game_state.player2_model_glb = load_gltf_model(player2_glb_path);
                game_state.has_player2_model = true;
                std::cout << "Loaded player2 model with " << game_state.player2_model_glb.meshes.size() << " mesh(es)";
                std::cout << " and " << game_state.player2_model_glb.textures.size() << " texture(s)\n";
            }
            else if (std::filesystem::exists(executable_dir / "assets" / "character" / "player2" / "damsel_character.glb"))
            {
                std::cout << "Loading player2 model (GLB) from executable directory\n";
                game_state.player2_model_glb = load_gltf_model(executable_dir / "assets" / "character" / "player2" / "damsel_character.glb");
                game_state.has_player2_model = true;
                std::cout << "Loaded player2 model with " << game_state.player2_model_glb.meshes.size() << " mesh(es)";
                std::cout << " and " << game_state.player2_model_glb.textures.size() << " texture(s)\n";
            }
            else
            {
                std::cerr << "Warning: Player2 model file not found\n";
            }
        }
        catch (const std::exception& ex)
        {
            std::cerr << "Warning: Failed to load player2 model: " << ex.what() << '\n';
        }
    }

    void load_player3_assets(const std::filesystem::path& executable_dir, 
                           const std::filesystem::path& source_dir,
                           game::GameState& game_state)
    {
        // Try to load player3 GLB model
        std::filesystem::path player3_glb_path = source_dir.parent_path() / "assets" / "character" / "player3" / "monk_character.glb";

        try
        {
            if (std::filesystem::exists(player3_glb_path))
            {
                std::cout << "Loading player3 model (GLB) from: " << player3_glb_path << std::endl;
                game_state.player3_model_glb = load_gltf_model(player3_glb_path);
                game_state.has_player3_model = true;
                std::cout << "Loaded player3 model with " << game_state.player3_model_glb.meshes.size() << " mesh(es)";
                std::cout << " and " << game_state.player3_model_glb.textures.size() << " texture(s)\n";
            }
            else if (std::filesystem::exists(executable_dir / "assets" / "character" / "player3" / "monk_character.glb"))
            {
                std::cout << "Loading player3 model (GLB) from executable directory\n";
                game_state.player3_model_glb = load_gltf_model(executable_dir / "assets" / "character" / "player3" / "monk_character.glb");
                game_state.has_player3_model = true;
                std::cout << "Loaded player3 model with " << game_state.player3_model_glb.meshes.size() << " mesh(es)";
                std::cout << " and " << game_state.player3_model_glb.textures.size() << " texture(s)\n";
            }
            else
            {
                std::cerr << "Warning: Player3 model file not found\n";
            }
        }
        catch (const std::exception& ex)
        {
            std::cerr << "Warning: Failed to load player3 model: " << ex.what() << '\n';
        }
    }

    void load_player4_assets(const std::filesystem::path& executable_dir, 
                           const std::filesystem::path& source_dir,
                           game::GameState& game_state)
    {
        // Try to load player4 GLB model
        std::filesystem::path player4_glb_path = source_dir.parent_path() / "assets" / "character" / "player4" / "scarecrow_target.glb";

        try
        {
            if (std::filesystem::exists(player4_glb_path))
            {
                std::cout << "Loading player4 model (GLB) from: " << player4_glb_path << std::endl;
                game_state.player4_model_glb = load_gltf_model(player4_glb_path);
                game_state.has_player4_model = true;
                std::cout << "Loaded player4 model with " << game_state.player4_model_glb.meshes.size() << " mesh(es)";
                std::cout << " and " << game_state.player4_model_glb.textures.size() << " texture(s)\n";
            }
            else if (std::filesystem::exists(executable_dir / "assets" / "character" / "player4" / "scarecrow_target.glb"))
            {
                std::cout << "Loading player4 model (GLB) from executable directory\n";
                game_state.player4_model_glb = load_gltf_model(executable_dir / "assets" / "character" / "player4" / "scarecrow_target.glb");
                game_state.has_player4_model = true;
                std::cout << "Loaded player4 model with " << game_state.player4_model_glb.meshes.size() << " mesh(es)";
                std::cout << " and " << game_state.player4_model_glb.textures.size() << " texture(s)\n";
            }
            else
            {
                std::cerr << "Warning: Player4 model file not found\n";
            }
        }
        catch (const std::exception& ex)
        {
            std::cerr << "Warning: Failed to load player4 model: " << ex.what() << '\n';
        }
    }
}

int main(int argc, char* argv[])
{
    try
    {
        std::cout << "Initializing window..." << std::endl;
        // Initialize window
        core::Window window(800, 600, "Pacman OpenGL");
        std::cout << "Window created successfully!" << std::endl;
        
        // Setup camera
        core::Camera camera;
        window.set_scroll_callback([&camera](double /*x_offset*/, double y_offset) {
            camera.adjust_fov(static_cast<float>(y_offset));
        });

        // Get executable directory - cross-platform approach
        std::filesystem::path executable_dir = std::filesystem::current_path();
        if (argc > 0 && argv[0] != nullptr)
        {
            std::error_code ec;
            std::filesystem::path exe_path(argv[0]);
            
            // On Windows, argv[0] might be relative or just the filename
            // On macOS/Linux, it's usually the full path or relative to current directory
            if (exe_path.is_absolute())
            {
                executable_dir = exe_path.parent_path();
            }
            else
            {
                // Try to resolve relative path
                auto resolved = std::filesystem::weakly_canonical(
                    std::filesystem::current_path() / exe_path, ec);
                if (!ec && std::filesystem::exists(resolved))
                {
                    executable_dir = resolved.parent_path();
                }
            }
        }
        
        // Fallback: also check common build output locations
        // This helps when running from IDE or different working directories
        if (!std::filesystem::exists(executable_dir / "shaders"))
        {
            // Try parent directory (for build/Debug or build/Release structures)
            std::filesystem::path parent_dir = executable_dir.parent_path();
            if (std::filesystem::exists(parent_dir / "shaders"))
            {
                executable_dir = parent_dir;
            }
            // Try build directory relative to source
            std::filesystem::path source_dir = std::filesystem::path(__FILE__).parent_path().parent_path();
            std::filesystem::path build_dir = source_dir / "build";
            if (std::filesystem::exists(build_dir / "shaders"))
            {
                executable_dir = build_dir;
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
        const GLint color_override_location = glGetUniformLocation(program, "uColorOverride");
        const GLint use_color_override_location = glGetUniformLocation(program, "uUseColorOverride");
        
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

        // Load player assets
        load_player_assets(executable_dir, source_dir, game_state);
        
        // Load player2 assets
        load_player2_assets(executable_dir, source_dir, game_state);
        
        // Load player3 assets
        load_player3_assets(executable_dir, source_dir, game_state);
        
        // Load player4 assets
        load_player4_assets(executable_dir, source_dir, game_state);

        // Load audio files
        std::filesystem::path audio_dir = executable_dir / "assets" / "audio";
        if (!std::filesystem::exists(audio_dir))
        {
            audio_dir = source_dir.parent_path() / "assets" / "audio";
        }
        
        if (game_state.audio_manager.is_available())
        {
            // Load BGM
            std::filesystem::path bgm_path = audio_dir / "bgm.mp3";
            if (!std::filesystem::exists(bgm_path))
            {
                bgm_path = audio_dir / "bgm.ogg";
            }
            if (!std::filesystem::exists(bgm_path))
            {
                bgm_path = audio_dir / "bgm.wav";
            }
            if (std::filesystem::exists(bgm_path))
            {
                game_state.audio_manager.load_music(bgm_path.string(), "bgm");
                game_state.audio_manager.play_music("bgm", -1); // Loop forever
            }
            else
            {
                std::cout << "Warning: BGM file not found in " << audio_dir << std::endl;
                std::cout << "  Looking for: bgm.mp3, bgm.ogg, or bgm.wav" << std::endl;
            }
            
            // Load sound effects (optional - will work even if files don't exist)
            std::vector<std::pair<std::string, std::string>> sounds = {
                {"dice_roll", "dice_roll.wav"},
                {"step", "step.wav"},
                {"ladder", "ladder.wav"},
                {"snake", "snake.wav"},
                {"minigame_start", "minigame_start.wav"},
                {"minigame_success", "minigame_success.wav"},
                {"minigame_fail", "minigame_fail.wav"}
            };
            
            for (const auto& [name, filename] : sounds)
            {
                std::filesystem::path sound_path = audio_dir / filename;
                if (std::filesystem::exists(sound_path))
                {
                    game_state.audio_manager.load_sound(sound_path.string(), name);
                }
            }
        }

        // Initialize text renderer
        game::RenderState render_state;
        render_state.program = program;
        render_state.mvp_location = mvp_location;
        render_state.use_texture_location = use_texture_location;
        render_state.texture_location = texture_location;
        render_state.dice_texture_mode_location = dice_texture_mode_location;
        render_state.color_override_location = color_override_location;
        render_state.use_color_override_location = use_color_override_location;

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
        std::cout << "Initializing game loop and renderer..." << std::endl;
        game::GameLoop game_loop(window, camera, game_state, render_state);
        game::Renderer renderer(render_state);
        std::cout << "Entering main game loop..." << std::endl;

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

