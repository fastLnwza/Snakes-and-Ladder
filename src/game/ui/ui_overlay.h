#pragma once

#include <glad/glad.h>

#include <string>

namespace core
{
    class Window;
}

struct TextRenderer;
struct DebugWarpState;

namespace game::player::dice
{
    struct DiceState;
}

namespace game::minigame
{
    struct PrecisionTimingState;
}

namespace game::minigame::tile_memory
{
    struct TileMemoryState;
}

namespace game::ui
{
    void render_ui_overlay(const core::Window& window,
                           GLuint program,
                           GLint mvp_location,
                           GLint use_texture_location,
                           GLint dice_texture_mode_location,
                           TextRenderer& text_renderer,
                           const game::player::dice::DiceState& dice_state,
                           const game::minigame::PrecisionTimingState& precision_state,
                           const game::minigame::tile_memory::TileMemoryState& tile_memory_state,
                           const DebugWarpState& debug_warp_state,
                           float minigame_message_timer,
                           const std::string& minigame_message,
                           float dice_display_timer,
                           int player_steps_remaining);
}



