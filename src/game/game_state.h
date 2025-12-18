#pragma once

#include "../core/types.h"
#include "../rendering/mesh.h"
#include "../rendering/text_renderer.h"
#include "../rendering/texture_loader.h"
#include "../rendering/gltf_loader.h"
#include "../rendering/obj_loader.h"
#include "../rendering/animation_player.h"
#include "../core/audio_manager.h"
#include "map/map_manager.h"
#include "player/player.h"
#include "player/dice/dice.h"
#include "minigame/qte_minigame.h"
#include "minigame/tile_memory_minigame.h"
#include "minigame/reaction_minigame.h"
#include "minigame/math_minigame.h"
#include "minigame/pattern_minigame.h"
#include "menu/menu_state.h"
#include "win/win_state.h"

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

        // Players (up to 4 players)
        std::array<game::player::PlayerState, 4> players{};
        int current_player_index = 0;  // 0-based index of current player
        int num_players = 1;  // Number of active players (1-4)
        float player_ground_y = 0.0f;
        float player_radius = 0.0f;
        Mesh sphere_mesh{};  // Fallback mesh if model is not loaded
        
        // Animation states for each player
        std::array<AnimationPlayerState, 4> player_animations{};
        GLTFModel player_model_glb{};  // Player1 model (GLB format)
        bool has_player_model = false;
        
        // Player2 model (GLB format)
        GLTFModel player2_model_glb{};
        bool has_player2_model = false;
        
        // Player3 model (GLB format)
        GLTFModel player3_model_glb{};
        bool has_player3_model = false;
        
        // Player4 model (GLB format)
        GLTFModel player4_model_glb{};
        bool has_player4_model = false;

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
        bool tile_memory_backspace_was_down = false;
        bool tile_memory_enter_was_down = false;
        std::array<bool, 6> pattern_previous_keys{};  // W, S, A, D, Backspace, Enter/Space
        std::string reaction_input_buffer;  // Buffer for number guessing input
        bool reaction_backspace_was_down = false;
        bool reaction_enter_was_down = false;
        std::array<bool, 10> math_digit_previous{};  // 0-9 for digit keys in math quiz
        bool math_backspace_was_down = false;
        bool math_enter_was_down = false;

        // Game state tracking
        int last_processed_tile = 0;
        std::array<int, 4> last_processed_tiles{};  // Track last processed tile for each player
        std::string minigame_message;
        float minigame_message_timer = 0.0f;
        float dice_display_timer = 0.0f;
        bool turn_finished = false;  // Flag to track if current player's turn is finished
        
        // Camera target position - updated only when switching players
        glm::vec3 camera_target_position = glm::vec3(0.0f, 0.0f, 0.0f);

        // Result tracking
        bool precision_result_applied = false;
        float precision_result_display_timer = 2.0f;
        bool tile_memory_result_applied = false;
        bool reaction_result_applied = false;
        bool math_result_applied = false;
        bool pattern_result_applied = false;

        // Debug
        DebugWarpState debug_warp_state;

        // Menu
        game::menu::MenuState menu_state;

        // Win screen
        game::win::WinState win_state;

        // Audio
        core::audio::AudioManager audio_manager;

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
        GLint color_override_location = -1;
        GLint use_color_override_location = -1;
        TextRenderer text_renderer{};
    };

    void initialize_game_state(GameState& state, const std::filesystem::path& executable_dir);
    void cleanup_game_state(GameState& state);
}

// Include menu_renderer and win_renderer after RenderState is defined
#include "menu/menu_renderer.h"
#include "win/win_renderer.h"
