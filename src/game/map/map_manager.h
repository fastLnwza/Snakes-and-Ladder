#pragma once

#include <glad/glad.h>
#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>

#include "../../core/types.h"
#include "../player/player.h"
#include "board.h"

// Forward declarations
namespace game::minigame
{
    struct PrecisionTimingState;
    struct ReactionState;
    struct MathQuizState;
    struct PatternMatchingState;
}

namespace game::minigame::tile_memory
{
    struct TileMemoryState;
}

namespace game::map
{
    struct MapData
    {
        ::Mesh mesh;
        float board_width = 0.0f;
        float board_height = 0.0f;
        float map_length = 0.0f;
        float map_min_dimension = 0.0f;
        int final_tile_index = 0;
    };

    // Initialize and build the map
    MapData initialize_map();

    // Check if player is on a ladder tile and warp them if needed
    // Returns true if player was warped, false otherwise
    bool check_and_apply_ladder(player::PlayerState& player_state, int current_tile, int& last_processed_tile);

    // Check if player is on a snake tile and warp them if needed (going backward)
    // Returns true if player was warped, false otherwise
    bool check_and_apply_snake(player::PlayerState& player_state, int current_tile, int& last_processed_tile);

    // Check tile activity and trigger appropriate minigame or special action
    // Returns true if a minigame was triggered or special action was applied
    bool check_tile_activity(int current_tile, 
                            int& last_processed_tile,
                            bool minigame_running,
                            bool tile_memory_active,
                            player::PlayerState& player_state,
                            game::minigame::PrecisionTimingState& minigame_state,
                            game::minigame::tile_memory::TileMemoryState& tile_memory_state,
                            game::minigame::ReactionState& reaction_state,
                            game::minigame::MathQuizState& math_state,
                            game::minigame::PatternMatchingState& pattern_state,
                            std::string& minigame_message,
                            float& minigame_message_timer,
                            std::array<bool, 10>& tile_memory_previous_keys,
                            bool& precision_space_was_down);

    // Render the map
    void render_map(const MapData& map_data, 
                   const glm::mat4& projection, 
                   const glm::mat4& view,
                   ::GLuint program,
                   ::GLint mvp_location);
}

