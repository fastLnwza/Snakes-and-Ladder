#pragma once

#include <string>
#include <vector>

namespace game::minigame::tile_memory
{
    enum class Phase
    {
        Inactive,
        ShowingSequence,
        WaitingInput,
        Result
    };

    struct TileMemoryState
    {
        Phase phase = Phase::Inactive;
        std::vector<int> sequence;
        std::vector<int> input_history;
        int highlight_index = 0;
        float highlight_timer = 0.0f;
        float highlight_interval = 0.7f;
        float input_timer = 0.0f;
        float input_time_limit = 6.0f;
        bool success = false;
        std::string display_text;
        std::string result_text;
        float result_timer = 0.0f;
        int bonus_steps = 0;
    };

    void start(TileMemoryState& state, int sequence_length = 3);
    void advance(TileMemoryState& state, float delta_time);
    void submit_choice(TileMemoryState& state, int tile_choice);
    bool is_running(const TileMemoryState& state);
    bool is_active(const TileMemoryState& state);
    bool is_result(const TileMemoryState& state);
    bool is_success(const TileMemoryState& state);
    std::string get_display_text(const TileMemoryState& state);
    int get_bonus_steps(const TileMemoryState& state);
    void reset(TileMemoryState& state);
}




