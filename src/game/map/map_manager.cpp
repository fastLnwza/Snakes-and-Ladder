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
#include <random>
#include <chrono>
#include <sstream>

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
                // Stop player movement after using ladder - don't continue walking
                player_state.steps_remaining = 0;
                player_state.is_stepping = false;
                return true;
            }
        }
        return false;
    }

    bool check_and_apply_snake(player::PlayerState& player_state, int current_tile, int& last_processed_tile)
    {
        for (const auto& link : BOARD_LINKS)
        {
            if (!link.is_ladder && link.start == current_tile)
            {
                // Found a snake - warp player to the end tile (going backward)
                player::warp_to_tile(player_state, link.end);
                last_processed_tile = link.end;
                // Stop player movement after using snake - don't continue walking
                player_state.steps_remaining = 0;
                player_state.is_stepping = false;
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
                minigame_message.clear();  // Clear minigame message to show math quiz text instead
                minigame_message_timer = 0.0f;
                return true;
            }
            else if (tile_activity == ActivityKind::PatternGame && current_tile != 0)
            {
                game::minigame::start_pattern_matching(pattern_state);
                minigame_message.clear();  // Clear minigame message to show pattern text instead
                minigame_message_timer = 0.0f;
                return true;
            }
            else if (tile_activity == ActivityKind::Slide && current_tile != 0)
            {
                // Slide: เดินเพิ่มไปอีก 1 ช่อง
                player_state.steps_remaining += 1;
                minigame_message = "Slide! +1 step";
                minigame_message_timer = 2.0f;
                return true;
            }
            else if (tile_activity == ActivityKind::Portal && current_tile != 0)
            {
                // Portal: สุ่มวาปไปช่องไหนก็ได้ (0-99)
                static std::mt19937 rng(static_cast<unsigned int>(std::chrono::steady_clock::now().time_since_epoch().count()));
                const int final_tile = BOARD_COLUMNS * BOARD_ROWS - 1;
                std::uniform_int_distribution<int> dist(0, final_tile);
                int random_tile = dist(rng);
                
                // ไม่วาปไปช่องเดิม
                while (random_tile == current_tile && final_tile > 0)
                {
                    random_tile = dist(rng);
                }
                
                player::warp_to_tile(player_state, random_tile);
                last_processed_tile = random_tile;
                player_state.steps_remaining = 0;
                player_state.is_stepping = false;
                
                std::ostringstream oss;
                oss << "Portal! Warped to tile " << (random_tile + 1);
                minigame_message = oss.str();
                minigame_message_timer = 2.0f;
                return true;
            }
            else if (tile_activity == ActivityKind::Trap && current_tile != 0)
            {
                // Trap: ข้ามเทิร์นเหมือน Skip Turn
                player::skip_turn(player_state);
                minigame_message = "Trap! Skip Turn!";
                minigame_message_timer = 2.0f;
                return true;
            }
            else if (tile_activity == ActivityKind::Bonus && current_tile != 0)
            {
                // Bonus: สุ่มว่าจะได้เดินเพิ่มอีกเท่าไหร่ (1-6)
                static std::mt19937 rng(static_cast<unsigned int>(std::chrono::steady_clock::now().time_since_epoch().count()));
                std::uniform_int_distribution<int> dist(1, 6);
                int bonus_steps = dist(rng);
                
                player_state.steps_remaining += bonus_steps;
                
                std::ostringstream oss;
                oss << "Bonus! +" << bonus_steps << " steps";
                minigame_message = oss.str();
                minigame_message_timer = 2.0f;
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
