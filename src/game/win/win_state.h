#pragma once

namespace game
{
    namespace win
    {
        struct WinState
        {
            bool is_active = false;  // Win screen is shown
            bool show_animation = false;  // Show win animation
            float animation_timer = 0.0f;  // Timer for animations
            int winner_player = 1;  // Which player won (1-based)
        };
    }
}

