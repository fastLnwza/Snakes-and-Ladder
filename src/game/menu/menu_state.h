#pragma once

namespace game
{
    namespace menu
    {
        struct MenuState
        {
            bool is_active = true;  // Menu is shown by default
            bool start_game = false;  // Set to true when user clicks start
            int num_players = 2;  // Number of players (1-4)
            bool use_ai = false;  // Whether to use AI players
            int selected_option = 0;  // 0 = num_players, 1 = use_ai (no option 2 for start button)
        };
    }
}


