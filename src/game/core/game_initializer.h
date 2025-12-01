#pragma once

#include "game_state.h"
#include <filesystem>

namespace game
{
    struct InitializationResult
    {
        bool success = false;
        std::string error_message;
    };
    
    InitializationResult initialize_game(GameState& state, int argc, char* argv[]);
    void cleanup_game(GameState& state);
}

