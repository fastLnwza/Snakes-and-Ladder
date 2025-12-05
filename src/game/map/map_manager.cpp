#include "map_manager.h"

#include "../../rendering/mesh.h"
#include "../minigame/qte_minigame.h"
#include "../minigame/tile_memory_minigame.h"
#include "../minigame/reaction_minigame.h"
#include "../minigame/math_minigame.h"
#include "../minigame/pattern_minigame.h"
#include "map_generator.h"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>

namespace game::map
{
    MapData initialize_map()
    {
        MapData data;
        
        const auto [map_vertices, map_indices] = build_snakes_ladders_map();
        data.mesh = create_mesh(map_vertices, map_indices);
        
        data.board_width = static_cast<float>(BOARD_COLUMNS) * TILE_SIZE;
        data.board_height = static_cast<float>(BOARD_ROWS) * TILE_SIZE;
        data.map_length = data.board_height;
        data.map_min_dimension = std::min(data.board_width, data.board_height);
        data.final_tile_index = BOARD_COLUMNS * BOARD_ROWS - 1;
        
        return data;
    }

    bool check_and_apply_ladder(player::PlayerState& player_state, int current_tile, int& last_processed_tile)
    {
        for (const auto& link : BOARD_LINKS)
        {
            if (link.is_ladder && link.start == current_tile)
            {
                // Found a ladder - warp player to the end tile
                player::warp_to_tile(player_state, link.end);
                last_processed_tile = link.end;
                return true;
            }
        }
        return false;
    }

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
                            bool& precision_space_was_down)
    {
        (void)last_processed_tile;  // Unused parameter
        if (!minigame_running && !tile_memory_active)
        {
            const ActivityKind tile_activity = classify_activity_tile(current_tile);
            
            if (tile_activity == ActivityKind::SkipTurn && current_tile != 0)
            {
                player::skip_turn(player_state);
                minigame_message = "Skip Turn!";
                minigame_message_timer = 2.0f;
                return true;
            }
            else if (tile_activity == ActivityKind::WalkBackward && current_tile != 0)
            {
                player::step_backward(player_state, 3);  // Walk backward 3 steps
                minigame_message = "Walk Backward 3 steps!";
                minigame_message_timer = 2.0f;
                return true;
            }
            else if (tile_activity == ActivityKind::MiniGame && current_tile != 0)
            {
                game::minigame::start_precision_timing(minigame_state);
                precision_space_was_down = false;
                minigame_message = "Precision Timing Challenge! Stop at 4.99";
                minigame_message_timer = 0.0f;
                return true;
            }
            else if (tile_activity == ActivityKind::MemoryGame && current_tile != 0)
            {
                game::minigame::tile_memory::start(tile_memory_state);
                tile_memory_previous_keys.fill(false);
                minigame_message = "จำลำดับ! ใช้ปุ่ม 1-9";
                minigame_message_timer = 0.0f;
                return true;
            }
            else if (tile_activity == ActivityKind::ReactionGame && current_tile != 0)
            {
                game::minigame::start_reaction(reaction_state);
                minigame_message.clear();  // Clear minigame message to show reaction text instead
                minigame_message_timer = 0.0f;
                return true;
            }
            else if (tile_activity == ActivityKind::MathGame && current_tile != 0)
            {
                game::minigame::start_math_quiz(math_state);
                minigame_message = "Math Quiz!";
                minigame_message_timer = 0.0f;
                return true;
            }
            else if (tile_activity == ActivityKind::PatternGame && current_tile != 0)
            {
                game::minigame::start_pattern_matching(pattern_state);
                minigame_message = "Pattern Matching!";
                minigame_message_timer = 0.0f;
                return true;
            }
        }
        return false;
    }

    void render_map(const MapData& map_data, 
                   const glm::mat4& projection, 
                   const glm::mat4& view,
                   ::GLuint program,
                   ::GLint mvp_location)
    {
        (void)program; // Unused parameter
        const glm::mat4 model(1.0f);
        const glm::mat4 mvp = projection * view * model;
        ::glUniformMatrix4fv(mvp_location, 1, GL_FALSE, glm::value_ptr(mvp));
        ::glBindVertexArray(map_data.mesh.vao);
        ::glDrawElements(GL_TRIANGLES, map_data.mesh.index_count, GL_UNSIGNED_INT, nullptr);
    }
}
