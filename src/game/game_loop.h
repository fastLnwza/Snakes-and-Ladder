#pragma once

#include "game_state.h"
#include "../core/window.h"
#include "../core/camera.h"

namespace game
{
    class GameLoop
    {
    public:
        GameLoop(core::Window& window, core::Camera& camera, GameState& game_state, RenderState& render_state);
        ~GameLoop() = default;

        void update(float delta_time);
        void render(const core::Camera& camera);

    private:
        void handle_input(float delta_time);
        void handle_ai_input(float delta_time);
        void update_game_logic(float delta_time);
        void update_minigames(float delta_time);
        void handle_minigame_results(float delta_time);
        void update_player_animations(float delta_time);

        core::Window& m_window;
        GameState& m_game_state;
        // Note: m_camera and m_render_state are stored for potential future use
        // They are currently passed as parameters to render() instead
        [[maybe_unused]] core::Camera& m_camera;
        [[maybe_unused]] RenderState& m_render_state;
    };
}



