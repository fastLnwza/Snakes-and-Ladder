#pragma once

#include "../../core/window.h"
#include <string>

// Forward declarations
namespace game { struct RenderState; }

namespace game
{
    namespace minigame
    {
        namespace menu
        {
            void render_minigame_menu(const core::Window& window, const void* render_state_ptr,
                                     const std::string& game_title, const std::string& game_description,
                                     const std::string& instruction_text, int bonus_steps);
        }
    }
}


