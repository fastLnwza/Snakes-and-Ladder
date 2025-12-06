#pragma once

namespace game
{
    namespace menu
    {
        struct MenuState
        {
            bool is_active = true;  // Menu is shown by default
            bool start_game = false;  // Set to true when user clicks start
        };
    }
}


