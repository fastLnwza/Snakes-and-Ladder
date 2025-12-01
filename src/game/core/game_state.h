#pragma once

#include "core/types.h"
#include "core/window.h"
#include "core/camera.h"
#include "game/player/player.h"
#include "game/player/dice/dice.h"
#include "game/minigame/qte_minigame.h"
#include "game/minigame/tile_memory_minigame.h"
#include "game/debug/debug_warp_state.h"
#include "rendering/geometry/mesh.h"
#include "rendering/loaders/gltf_loader.h"
#include "rendering/loaders/obj_loader.h"
#include "rendering/loaders/texture_loader.h"
#include "rendering/text/text_renderer.h"

#include <glm/glm.hpp>
#include <array>
#include <string>

namespace game
{
    struct GameState
    {
        // Core systems
        core::Window* window = nullptr;
        core::Camera camera;
        
        // Shader program
        GLuint shader_program = 0;
        GLint mvp_location = -1;
        GLint use_texture_location = -1;
        GLint texture_location = -1;
        GLint dice_texture_mode_location = -1;
        
        // Map
        Mesh map_mesh{};
        float board_width = 0.0f;
        float board_height = 0.0f;
        float map_length = 0.0f;
        int final_tile_index = 0;
        
        // Player
        game::player::PlayerState player_state{};
        Mesh player_mesh{};
        float player_ground_y = 0.0f;
        float player_radius = 0.0f;
        
        // Dice
        game::player::dice::DiceState dice_state{};
        GLTFModel dice_model_glb{};
        OBJModel dice_model_obj{};
        bool has_dice_model = false;
        bool is_obj_format = false;
        Texture dice_texture{};
        bool has_dice_texture = false;
        
        // Minigames
        game::minigame::PrecisionTimingState minigame_state{};
        game::minigame::tile_memory::TileMemoryState tile_memory_state{};
        bool precision_space_was_down = false;
        std::array<bool, 9> tile_memory_previous_keys{};
        bool precision_result_applied = false;
        float precision_result_display_timer = 3.0f;
        bool tile_memory_result_applied = false;
        
        // UI
        TextRenderer text_renderer{};
        std::string minigame_message;
        float minigame_message_timer = 0.0f;
        float dice_display_timer = 0.0f;
        
        // Game logic
        int last_processed_tile = 0;
        
        // Debug
        DebugWarpState debug_warp_state{};
    };
}

