#pragma once

#include "../core/types.h"
#include "../rendering/mesh.h"
#include "../rendering/text_renderer.h"
#include "../rendering/texture_loader.h"
#include "../rendering/gltf_loader.h"
#include "../rendering/obj_loader.h"
#include "map/map_manager.h"
#include "player/player.h"
#include "player/dice/dice.h"
#include "minigame/qte_minigame.h"
#include "minigame/tile_memory_minigame.h"
#include "minigame/reaction_minigame.h"
#include "minigame/math_minigame.h"
#include "minigame/pattern_minigame.h"

#include <array>
#include <string>

namespace game
{
    struct DebugWarpState
    {
        bool active = false;
        std::string buffer;
        std::array<bool, 10> digit_previous{};
        bool prev_toggle = false;
        bool prev_enter = false;
        bool prev_backspace = false;
        float notification_timer = 0.0f;
        std::string notification;
    };

    struct GameState
    {
        // Map
        game::map::MapData map_data;
        float map_length = 0.0f;
        float map_min_dimension = 0.0f;
        int final_tile_index = 0;

        // Player
        game::player::PlayerState player_state;
        float player_ground_y = 0.0f;
        float player_radius = 0.0f;
        Mesh sphere_mesh{};

        // Dice
        game::player::dice::DiceState dice_state;
        GLTFModel dice_model_glb{};
        OBJModel dice_model_obj{};
        bool has_dice_model = false;
        bool is_obj_format = false;
        Texture dice_texture{};
        bool has_dice_texture = false;

        // Minigames
        game::minigame::PrecisionTimingState minigame_state;
        game::minigame::tile_memory::TileMemoryState tile_memory_state;
        game::minigame::ReactionState reaction_state;
        game::minigame::MathQuizState math_state;
        game::minigame::PatternMatchingState pattern_state;

        // Minigame input tracking
        bool precision_space_was_down = false;
        std::array<bool, 10> tile_memory_previous_keys{};  // 0-9 for digit keys
        std::array<bool, 4> pattern_previous_keys{};
        std::string reaction_input_buffer;  // Buffer for number guessing input
        bool reaction_backspace_was_down = false;
        bool reaction_enter_was_down = false;
        std::array<bool, 10> math_digit_previous{};  // 0-9 for digit keys in math quiz
        bool math_backspace_was_down = false;
        bool math_enter_was_down = false;

        // Game state tracking
        int last_processed_tile = 0;
        std::string minigame_message;
        float minigame_message_timer = 0.0f;
        float dice_display_timer = 0.0f;

        // Result tracking
        bool precision_result_applied = false;
        float precision_result_display_timer = 3.0f;
        bool tile_memory_result_applied = false;
        bool reaction_result_applied = false;
        bool math_result_applied = false;
        bool pattern_result_applied = false;

        // Debug
        DebugWarpState debug_warp_state;

        // Timing
        float last_time = 0.0f;
    };

    struct RenderState
    {
        GLuint program = 0;
        GLint mvp_location = -1;
        GLint use_texture_location = -1;
        GLint texture_location = -1;
        GLint dice_texture_mode_location = -1;
        TextRenderer text_renderer{};
    };

    void initialize_game_state(GameState& state, const std::filesystem::path& executable_dir);
    void cleanup_game_state(GameState& state);
}

