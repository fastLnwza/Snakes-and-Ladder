#pragma once

#include "../../core/window.h"
#include <filesystem>
#include <glm/glm.hpp>

// Forward declarations
namespace game { struct RenderState; }
namespace game { namespace win { struct WinState; } }

namespace game
{
    namespace win
    {
        void render_win_screen(const core::Window& window, const void* render_state_ptr,
                              const WinState& win_state);
    }
}




















