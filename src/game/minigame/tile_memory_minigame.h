#pragma once

#include <string>
#include <vector>

namespace game::minigame::tile_memory
{
    enum class Phase
    {
        Inactive,
        ShowingTitle,
        ShowingSequence,
        WaitingInput,
        Result
    };

    struct TileMemoryState
    {
        Phase phase = Phase::Inactive;
        float title_timer = 0.0f;
        float title_duration = 5.0f;
        std::vector<int> sequence;
        std::vector<int> input_history;
        std::string input_buffer;  // Buffer for input (1-9)
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
        int current_round = 0;  // 1 = first round (3 digits), 2 = second round (4 digits)
    };

    void start(TileMemoryState& state, int sequence_length = 3);
    void advance(TileMemoryState& state, float delta_time);
    void submit_choice(TileMemoryState& state, int tile_choice);
    void add_digit(TileMemoryState& state, char digit);
    void remove_digit(TileMemoryState& state);
    void submit_buffer(TileMemoryState& state);
    bool is_running(const TileMemoryState& state);
    bool is_active(const TileMemoryState& state);
    bool is_result(const TileMemoryState& state);
    bool is_success(const TileMemoryState& state);
    std::string get_display_text(const TileMemoryState& state);
    int get_bonus_steps(const TileMemoryState& state);
    void reset(TileMemoryState& state);
}




