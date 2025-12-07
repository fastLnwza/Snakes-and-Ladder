#include "game_state.h"

#include "../core/camera.h"
#include "../game/map/board.h"
#include "../game/player/player.h"
#include "../rendering/primitives.h"
#include "../rendering/mesh.h"

namespace game
{
    void initialize_game_state(GameState& state, const std::filesystem::path& /*executable_dir*/)
    {
        using namespace game::map;
        using namespace game::player;

        // Build map
        state.map_data = initialize_map();
        state.map_length = state.map_data.map_length;
        state.map_min_dimension = state.map_data.map_min_dimension;
        state.final_tile_index = state.map_data.final_tile_index;

        // Create player
        state.player_radius = std::max(0.4f, 0.025f * state.map_min_dimension);
        const auto sphere_result = ::build_sphere(state.player_radius, 32, 16, {1.0f, 0.9f, 0.1f});
        state.sphere_mesh = ::create_mesh(sphere_result.first, sphere_result.second);

        state.player_ground_y = state.player_radius;
        // Initialize all players (will be activated based on num_players from menu)
        for (int i = 0; i < 4; ++i)
        {
            initialize(state.players[i], tile_center_world(0), state.player_ground_y, state.player_radius);
        }
        state.current_player_index = 0;
        state.num_players = 1;  // Default to 1 player, will be updated from menu
        state.last_processed_tile = get_current_tile(state.players[0]);
        state.last_processed_tiles.fill(0);
        state.turn_finished = false;
        // Initialize camera target position to first player's position
        state.camera_target_position = get_position(state.players[0]);

        // Initialize dice state
        using namespace game::player::dice;
        const glm::vec3 dice_position = tile_center_world(0) + glm::vec3(0.0f, state.player_ground_y + 3.0f, 0.0f);
        initialize(state.dice_state, dice_position);
        state.dice_state.scale = state.player_radius * 0.167f;
        state.dice_state.rotation = glm::vec3(45.0f, 45.0f, 0.0f);
    }

    void cleanup_game_state(GameState& state)
    {
        destroy_mesh(state.map_data.mesh);
        destroy_mesh(state.sphere_mesh);
        if (state.has_dice_texture)
        {
            destroy_texture(state.dice_texture);
        }
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
        if (state.has_player_model)
        {
            if (!state.player_model_glb.meshes.empty())
            {
                destroy_gltf_model(state.player_model_glb);
            }
        }
    }
}

