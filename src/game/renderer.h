#pragma once

#include "game_state.h"
#include "../core/camera.h"
#include "../core/window.h"
#include "../rendering/text_renderer.h"
#include "../rendering/gltf_loader.h"
#include "../rendering/obj_loader.h"

#include <glm/glm.hpp>

namespace game
{
    class Renderer
    {
    public:
        Renderer(const RenderState& render_state);
        ~Renderer() = default;

        void render(const core::Window& window, const core::Camera& camera, const GameState& game_state);

    private:
        void render_map(const glm::mat4& projection, const glm::mat4& view, const GameState& game_state);
        void render_player(const glm::mat4& projection, const glm::mat4& view, const GameState& game_state);
        void render_dice(const glm::mat4& projection, const glm::mat4& view, const GameState& game_state);
        void render_ui(const core::Window& window, const GameState& game_state);

        const RenderState& m_render_state;
    };
}

